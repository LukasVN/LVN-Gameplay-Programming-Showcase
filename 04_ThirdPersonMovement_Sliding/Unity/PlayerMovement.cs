using UnityEngine;

[RequireComponent(typeof(CharacterController))]
public class PlayerMovement : MonoBehaviour
{
    [Header("Movement Settings")]
    [SerializeField] private float walkSpeed = 2f;
    [SerializeField] private float runSpeed = 5f;
    [SerializeField] private float rotationSpeed = 4f;
    [SerializeField] private float gravity = -20f;
    [SerializeField] private float jumpForce = 10f;
    [SerializeField] private float flipForce = 8f;
    private Vector3 _velocity;

    [Header("Falling Settings")]
    [SerializeField] private float fallingVelocityThreshold = -1f;
    [SerializeField] private float fallingRayLength = 0.1f;
    [SerializeField] private float fallGraceTime = 0.05f;
    private bool _isFalling;
    private float _fallTimer;

    [Header("Jump Settings")]
    [SerializeField] private bool allowDoubleJump = true;
    [SerializeField] private float jumpInputBufferTime = 0.01f;
    private bool _jumpInputQueued;
    private float _jumpInputTimer;
    private bool _jumpPending;
    private int _jumpCount;
    private bool _isFlipping;

    [Header("Crouch Settings")]
    [SerializeField] private float ceilingCheckOffset = 0.15f;
    [SerializeField] private float crouchHeight = 1.0f;
    [SerializeField] private float crouchCenterY = 0.57f;
    [SerializeField] private float standHeight = 1.67f;
    [SerializeField] private float standCenterY = 0.9f;
    [SerializeField] private float crouchSpeed = 1.67f;
    [SerializeField] private float crouchBackwardsSpeed = 1.5f;
    private bool _isCrouching;

    [Header("Prone Settings")]
    [SerializeField] private float proneHeight = 0.4f;
    [SerializeField] private float proneCenterY = 0.2f;
    [SerializeField] private float proneSpeed = 1.25f;
    [SerializeField] private float proneBackwardsSpeed = 1.0f;
    private bool _isProning;

    [Header("Slide Settings")]
    [SerializeField] private float slideHeight = 0.2f;
    [SerializeField] private float slideCenterY = 0.4f;
    private bool _isSliding;
    [SerializeField] private float slideFallGraceTime = 1f;
    [SerializeField] private float slideSlopeBoost = 15f;
    [SerializeField] private float slideFriction = 1f;
    [SerializeField] private float minSlideSpeed = 3.5f;
    [SerializeField] private float flatSlideBoost = 2.5f;
    private float _slideFallTimer;
    private Vector3 _slideVelocity;


    #if UNITY_EDITOR
    [Header("Unity Editor Settings")]
    [SerializeField] private Color standUpCheckColor = Color.blue;
    #endif
    
    private CharacterController _controller;
    private Animator _animator;
    private Transform _mainCamera;

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
        bool runPressed = InputManager.Instance.IsRunning;
        bool dancePressed = InputManager.Instance.IsDancing;
        bool crouchTogglePressed = InputManager.Instance.IsCrouching;
        bool crouchButtonPressed = InputManager.Instance.CrouchButtonPressed;
        bool proneTogglePressed = InputManager.Instance.IsProning;
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

            if (_jumpInputQueued && !_jumpPending && !_isCrouching && !_isProning)
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
                && _jumpCount == 0
                && !_isCrouching
                && !_isProning)
            {
                _animator.SetTrigger("AirJumpTrigger");
                _jumpPending = true;
                _jumpInputQueued = false;
                _jumpCount++;
            }
        }

        bool CancelSlideCondition() =>
            !crouchButtonPressed || _slideVelocity.magnitude < minSlideSpeed || (!grounded && _slideFallTimer > slideFallGraceTime);

        void CancelSlide()
        {
            _isSliding = false;
            _animator.SetBool("IsSliding", false);
            _animator.ResetTrigger("Slide_Trigger");

            bool standAllowed = CanStandUp();
            bool crouchAllowed = CanCrouchUp();

            if (standAllowed && crouchAllowed)
            {
                _controller.height = standHeight;
                _controller.center = new Vector3(0f, standCenterY, 0f);
                _isCrouching = false;
                _isProning = false;
                _animator.SetBool("IsCrouching", false);
                _animator.SetBool("IsProning", false);
                _animator.ResetTrigger("CrouchTrigger");
            }
            else if (crouchAllowed)
            {
                _controller.height = crouchHeight;
                _controller.center = new Vector3(0f, crouchCenterY, 0f);
                _isCrouching = true;
                _isProning = false;
                _animator.SetBool("IsCrouching", true);
                _animator.SetBool("IsProning", false);
                _animator.SetTrigger("CrouchTrigger");
            }
            else
            {
                _controller.height = proneHeight;
                _controller.center = new Vector3(0f, proneCenterY, 0f);
                _isCrouching = true; // required for prone animation with my setup
                _isProning = true;
                _animator.SetBool("IsCrouching", true); // required for prone animation with my setup
                _animator.SetBool("IsProning", true);
                _animator.SetTrigger("ProneTrigger");
            }
        }

        bool airborneSlideAllowed = !grounded && _velocity.y < 0f;
        bool canSlide = runPressed && input.magnitude > 0.1f && !_isSliding && !_isCrouching && !_isProning && (grounded || airborneSlideAllowed);
        if (!_isSliding && crouchButtonPressed && canSlide)
        {
            _isSliding = true;
            _slideVelocity = moveDir * runSpeed;
            _controller.height = slideHeight;
            _controller.center = new Vector3(0f, slideCenterY, 0f);
            _animator.SetBool("IsSliding", true);
            _animator.ResetTrigger("Slide_Trigger");
            _animator.SetTrigger("Slide_Trigger");
            _slideFallTimer = 0f;
        }

        if (_isSliding)
        {
            Vector3 groundNormal = Vector3.up;
            if (Physics.Raycast(transform.position, Vector3.down, out RaycastHit hit, 1.5f))
            {
                groundNormal = hit.normal;
            }

            Vector3 slopeDir = Vector3.ProjectOnPlane(Vector3.down, groundNormal).normalized;
            _slideVelocity += slopeDir * slideSlopeBoost * Time.deltaTime;
            _slideVelocity += moveDir * flatSlideBoost * Time.deltaTime;
            _slideVelocity = Vector3.Lerp(_slideVelocity, Vector3.zero, slideFriction * Time.deltaTime);

            if (!grounded)
            {
                _slideFallTimer += Time.deltaTime;
            }
            else
            {
                _slideFallTimer = 0f;
            }

            if (CancelSlideCondition())
            {
                CancelSlide();
            }
        }

        if (proneTogglePressed && _isCrouching && !_isProning)
        {
            _controller.height = proneHeight;
            _controller.center = new Vector3(0f, proneCenterY, 0f);
            _isProning = true;
            _animator.SetBool("IsProning", true);
            _animator.SetTrigger("ProneTrigger");
        }
        else if (proneTogglePressed && _isProning)
        {
            if (CanCrouchUp())
            {
                _controller.height = crouchHeight;
                _controller.center = new Vector3(0f, crouchCenterY, 0f);
                _isProning = false;
                _isCrouching = true;
                _animator.SetBool("IsProning", false);
                _animator.SetBool("IsCrouching", true);
                _animator.SetTrigger("ProneTrigger");
            }
        }

        if (crouchTogglePressed && !runPressed && !_isProning && !_isSliding)
        {
            if (_isCrouching)
            {
                if (CanStandUp())
                {
                    _controller.height = standHeight;
                    _controller.center = new Vector3(0f, standCenterY, 0f);
                    _animator.ResetTrigger("CrouchTrigger");
                    _isCrouching = false;
                    _animator.SetBool("IsCrouching", false);
                }
                else
                {
                    return;
                }
            }
            else
            {
                _isCrouching = true;
                _controller.height = crouchHeight;
                _controller.center = new Vector3(0f, crouchCenterY, 0f);
                _animator.SetTrigger("CrouchTrigger");
                _animator.SetBool("IsCrouching", true);
            }
        }

        bool movingBackward = Vector3.Dot(new Vector3(moveDir.x, 0f, moveDir.z), _mainCamera.forward) < -0.1f;

        float targetSpeed;
        if (_isSliding)
        {
            targetSpeed = _slideVelocity.magnitude;
        }
        else if (_isProning)
        {
            targetSpeed = movingBackward ? proneBackwardsSpeed : proneSpeed;
        }
        else if (_isCrouching)
        {
            targetSpeed = movingBackward ? crouchBackwardsSpeed : crouchSpeed;
        }
        else if (runPressed && input.y >= 0f && !movingBackward)
        {
            targetSpeed = runSpeed;
        }
        else
        {
            targetSpeed = movingBackward ? walkSpeed * 0.85f : walkSpeed;
        }

        Vector3 horizontalVelocity = moveDir * targetSpeed;
        Vector3 finalVelocity = _isSliding
            ? (_slideVelocity + Vector3.up * _velocity.y) * Time.deltaTime
            : (horizontalVelocity + Vector3.up * _velocity.y) * Time.deltaTime;

        _controller.Move(finalVelocity);

        if (dancePressed && isIdle && !_animator.GetBool("IsFalling") && !_animator.GetBool("IsDancing"))
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
            Quaternion targetRot = movingBackward ? Quaternion.LookRotation(-moveDir) : Quaternion.LookRotation(moveDir);
            transform.rotation = Quaternion.Slerp(transform.rotation, targetRot, rotationSpeed * Time.deltaTime);
        }

        bool walkingAnim = input.magnitude > 0.1f && (!runPressed || _isCrouching || _isProning);

        _animator.SetBool("IsWalking", walkingAnim);
        _animator.SetBool("IsRunning", !_isCrouching && !_isProning && runPressed && input.magnitude > 0.1f && !movingBackward);
        _animator.SetBool("IsWalkingBackwards", movingBackward);
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
            _isCrouching = false;
            _animator.SetBool("IsCrouching", false);
        }
    }

    public void Jump()
    {
        if (_isCrouching) return;

        _velocity.y = 0;
        _velocity.y = jumpForce;
        _animator.SetBool("IsJumping", true);
        _jumpPending = false;
        _jumpInputQueued = false;
    }

    public void Jump(float customJumpForce)
    {
        if (_isCrouching) return;

        _velocity.y = 0;
        _velocity.y = customJumpForce;
        _animator.SetBool("IsJumping", true);
        _jumpPending = false;
        _jumpInputQueued = false;
    }

    public void BeginFlip()
    {
        if (_isCrouching) return;

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

    private bool CanStandUp()
    {
        float checkDistance = (standHeight - crouchHeight) - ceilingCheckOffset;
        Vector3 origin = transform.position + Vector3.up * crouchHeight;
        float radius = _controller.radius * 0.95f;

        return !Physics.SphereCast(origin, radius, Vector3.up, out _, checkDistance);
    }

    private bool CanCrouchUp()
    {
        float checkDistance = (crouchHeight - proneHeight) - ceilingCheckOffset;
        Vector3 origin = transform.position + Vector3.up * proneHeight;
        float radius = _controller.radius * 0.95f;

        return !Physics.SphereCast(origin, radius, Vector3.up, out _, checkDistance);
    }

    #if UNITY_EDITOR
    private void OnDrawGizmosSelected()
    {
        if (_controller == null) return;

        float checkDistance = (standHeight - crouchHeight) - ceilingCheckOffset;
        Vector3 origin = transform.position + Vector3.up * crouchHeight;
        float radius = _controller.radius * 0.95f;

        Gizmos.color = standUpCheckColor;
        Gizmos.DrawWireSphere(origin, radius);
        Gizmos.DrawLine(origin, origin + Vector3.up * checkDistance);
    }
    #endif

}
