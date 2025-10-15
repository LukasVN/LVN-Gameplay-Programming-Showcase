using UnityEngine;

[RequireComponent(typeof(CharacterController))]
public class PlayerMovement : MonoBehaviour
{
    [Header("Movement Settings")]
    public float walkSpeed = 3f;
    public float runSpeed = 6f;
    public float rotationSpeed = 10f;
    public float gravity = -9.81f;
    public float fallingVelocityThreshold = -1f;
    public float fallingRayLength = 0.6f;

    [Header("Falling Settings")]
    public float fallGraceTime = 0.15f;

    private CharacterController _controller;
    private Animator _animator;
    private Vector3 _velocity;
    private Transform _mainCamera;
    private bool _isFalling;
    private float _fallTimer = 0f;

    private void Start()
    {
        _controller = GetComponent<CharacterController>();
        _animator = GetComponentInChildren<Animator>();
        _mainCamera = Camera.main.transform;
    }

    private void Update()
    {
        HandleMovement();
        UpdateFalling();
    }

    private void HandleMovement()
    {
        Vector2 input = InputManager.Instance.MoveInput;
        bool isRunningInput = InputManager.Instance.IsRunning;
        bool isDancingInput = InputManager.Instance.IsDancing;
        bool isIdle = input.magnitude < 0.1f;

        // Camera-relative movement
        Vector3 camForward = _mainCamera.forward;
        Vector3 camRight = _mainCamera.right;
        camForward.y = 0f;
        camRight.y = 0f;
        Vector3 moveDir = camForward * input.y + camRight * input.x;
        moveDir.Normalize();

        bool grounded = (_controller.collisionFlags & CollisionFlags.Below) != 0;
        if (grounded && _velocity.y < 0)
        {
            _velocity.y = Mathf.Max(_velocity.y, -2f);
        }
        else
        {
            _velocity.y += gravity * Time.deltaTime;
        }

        bool canRun = isRunningInput && input.y > 0.1f;
        float targetSpeed = canRun ? runSpeed : walkSpeed;
        Vector3 horizontalVelocity = moveDir * targetSpeed;

        Vector3 finalVelocity = (horizontalVelocity + Vector3.up * _velocity.y) * Time.deltaTime;
        _controller.Move(finalVelocity);

        // Dancing logic
        if (isDancingInput && isIdle && !_animator.GetBool("IsFalling") && !_animator.GetBool("IsDancing"))
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

        // Animation states
        bool movingBackwardAnim = Vector3.Dot(new Vector3(moveDir.x, 0f, moveDir.z), _mainCamera.forward) < -0.1f;
        bool walkingAnim = input.magnitude > 0.1f && !canRun && !movingBackwardAnim;
        bool runningAnim = canRun && input.magnitude > 0.1f && !movingBackwardAnim;

        _animator.SetBool("IsWalking", walkingAnim);
        _animator.SetBool("IsRunning", runningAnim);
        _animator.SetBool("IsWalkingBackwards", movingBackwardAnim);
    }

    private void UpdateFalling()
    {
        // Ground check: CharacterController OR raycast
        bool grounded = (_controller.collisionFlags & CollisionFlags.Below) != 0 ||
                        Physics.Raycast(transform.position, Vector3.down, fallingRayLength);

        if (!grounded)
            _fallTimer += Time.deltaTime;
        else
            _fallTimer = 0f;

        bool currentlyFalling = _fallTimer > fallGraceTime && _velocity.y <= fallingVelocityThreshold;

        if (currentlyFalling && !_isFalling)
        {
            _isFalling = true;
            _animator.SetBool("IsFalling", true);
            _animator.SetTrigger("IsFallingTrigger");
        }
        else if (!currentlyFalling && _isFalling)
        {
            _isFalling = false;
            _animator.SetBool("IsFalling", false);
        }
    }
}
