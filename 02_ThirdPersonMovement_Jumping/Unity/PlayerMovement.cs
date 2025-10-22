using UnityEngine;

[RequireComponent(typeof(CharacterController))]
public class PlayerMovement : MonoBehaviour
{
    [Header("Movement Settings")]
    [SerializeField] private float walkSpeed = 2f;
    [SerializeField] private float runSpeed = 5f;
    [SerializeField] private float rotationSpeed = 4f;

    [Header("Falling Settings")]
    [SerializeField] private float fallingVelocityThreshold = -1f;
    [SerializeField] private float fallingRayLength = 0.1f;
    [SerializeField] private float fallGraceTime = 0.05f;

    [Header("Jump Settings")]
    [SerializeField] private float gravity = -20f;
    [SerializeField] private float jumpForce = 10f;
    [SerializeField] private float flipForce = 8f;
    [SerializeField] private bool allowDoubleJump = true;
    [SerializeField] private float jumpInputBufferTime = 0.01f;

    private CharacterController _controller;
    private Animator _animator;
    private Transform _mainCamera;

    private Vector3 _velocity;
    private bool _isFalling;
    private bool _isFlipping;
    private bool _jumpPending;
    private int _jumpCount;

    private float _fallTimer;
    private bool _jumpInputQueued;
    private float _jumpInputTimer;

    private void Start()
    {
        _controller = GetComponent<CharacterController>();
        _animator = GetComponentInChildren<Animator>();
        _mainCamera = Camera.main.transform;
    }

    private void Update()
    {
        QueueJumpInput();
        HandleMovement();
        UpdateFalling();
    }

    private bool IsGrounded()
    {
        return (_controller.collisionFlags & CollisionFlags.Below) != 0
               || Physics.Raycast(transform.position, Vector3.down, fallingRayLength);
    }

    private void QueueJumpInput()
    {
        if (InputManager.Instance.IsJumping)
        {
            _jumpInputQueued = true;
            _jumpInputTimer = jumpInputBufferTime;
        }

        if (_jumpInputQueued)
        {
            _jumpInputTimer -= Time.deltaTime;
            if (_jumpInputTimer <= 0f)
            {
                _jumpInputQueued = false;
            }
        }
    }

    private void HandleMovement()
    {
        Vector2 input = InputManager.Instance.MoveInput;
        bool isRunning = InputManager.Instance.IsRunning;
        bool isDancing = InputManager.Instance.IsDancing;
        bool isIdle = input.magnitude < 0.1f;

        Vector3 camForward = _mainCamera.forward;
        Vector3 camRight = _mainCamera.right;
        camForward.y = 0f;
        camRight.y = 0f;
        Vector3 moveDir = camForward * input.y + camRight * input.x;
        moveDir.Normalize();

        bool grounded = IsGrounded();

        if (grounded && _velocity.y < 0)
        {
            _velocity.y = Mathf.Max(_velocity.y, -2f);
            _jumpCount = 0;
            _isFlipping = false;
            _animator.SetBool("IsJumping", false);
            _animator.SetBool("IsFalling", false);

            if (_jumpInputQueued && !_jumpPending)
            {
                _animator.SetTrigger("JumpTrigger");
                _jumpPending = true;
                _jumpInputQueued = false;
                _jumpCount++;
            }
        }
        else
        {
            _velocity.y += gravity * Time.deltaTime;

            bool jumpPressed = _jumpInputQueued || InputManager.Instance.IsJumping;

            if (allowDoubleJump
                && jumpPressed
                && !_jumpPending
                && !_isFlipping
                && !grounded
                && _jumpCount == 0)
            {
                _animator.SetTrigger("AirJumpTrigger");
                _jumpPending = true;
                _jumpInputQueued = false;
                _jumpCount++;
            }
        }

        float targetSpeed = isRunning && input.y >= 0f ? runSpeed : walkSpeed;
        Vector3 horizontalVelocity = moveDir * targetSpeed;
        Vector3 finalVelocity = (horizontalVelocity + Vector3.up * _velocity.y) * Time.deltaTime;
        _controller.Move(finalVelocity);

        if (isDancing && isIdle && !_animator.GetBool("IsFalling") && !_animator.GetBool("IsDancing"))
        {
            _animator.SetBool("IsDancing", true);
            _animator.SetTrigger("IsDancingTrigger");
        }
        else if (!isIdle || _animator.GetBool("IsFalling"))
        {
            _animator.SetBool("IsDancing", false);
        }

        if (moveDir.magnitude > 0.1f)
        {
            bool movingBackward = Vector3.Dot(new Vector3(moveDir.x, 0f, moveDir.z), _mainCamera.forward) < -0.1f;
            Quaternion targetRot = movingBackward ? Quaternion.LookRotation(-moveDir) : Quaternion.LookRotation(moveDir);
            transform.rotation = Quaternion.Slerp(transform.rotation, targetRot, rotationSpeed * Time.deltaTime);
        }

        bool movingBackwardAnim = Vector3.Dot(new Vector3(moveDir.x, 0f, moveDir.z), _mainCamera.forward) < -0.1f;
        bool walkingAnim = input.magnitude > 0.1f && !isRunning && !movingBackwardAnim;
        bool runningAnim = isRunning && input.magnitude > 0.1f && !movingBackwardAnim;

        _animator.SetBool("IsWalking", walkingAnim);
        _animator.SetBool("IsRunning", runningAnim);
        _animator.SetBool("IsWalkingBackwards", movingBackwardAnim);
    }

    private void UpdateFalling()
    {
        bool grounded = IsGrounded();

        if (grounded)
        {
            _isFalling = false;
            _animator.SetBool("IsFalling", false);
            _fallTimer = 0f;
            _jumpInputTimer = 0f;
            _jumpInputQueued = false;
            _jumpPending = false;
            return;
        }

        _fallTimer += Time.deltaTime;

        bool currentlyFalling = _fallTimer > fallGraceTime && _velocity.y <= fallingVelocityThreshold;

        if (currentlyFalling && !_isFalling && !_isFlipping)
        {
            _isFalling = true;
            _animator.SetBool("IsFalling", true);
        }
    }

    public void Jump()
    {
        _velocity.y = 0;
        _velocity.y = jumpForce;
        _animator.SetBool("IsJumping", true);
        _jumpPending = false;
        _jumpInputQueued = false;
    }

    public void Jump(float customJumpForce)
    {
        _velocity.y = 0;
        _velocity.y = customJumpForce;
        _animator.SetBool("IsJumping", true);
        _jumpPending = false;
        _jumpInputQueued = false;
    }

    public void BeginFlip()
    {
        _isFlipping = true;
        _animator.SetBool("IsFlipping", true);
        _animator.SetBool("IsFalling", true);
        _jumpInputQueued = false;
        Jump(flipForce);
    }

    public void EndFlip()
    {
        _isFlipping = false;
        _animator.SetBool("IsFlipping", false);
        _animator.SetBool("IsJumping", false);
    }
}
