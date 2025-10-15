using UnityEngine;
using UnityEngine.InputSystem;

public class InputManager : MonoBehaviour
{
    public static InputManager Instance { get; private set; }

    private PlayerInput _playerInput;
    private InputAction _move;
    private InputAction _look;
    private InputAction _run;
    private InputAction _dance;

    public Vector2 MoveInput { get; private set; }
    public Vector2 LookInput { get; private set; }
    public bool IsRunning { get; private set; }
    public bool IsDancing { get; private set; }

    private void Awake()
    {
        if (Instance != null && Instance != this)
        {
            Destroy(gameObject);
            return;
        }
        Instance = this;

        _playerInput = GetComponent<PlayerInput>();

        _move = _playerInput.actions["Move"];
        _look = _playerInput.actions["Look"];
        _run = _playerInput.actions["Run"];
        _dance = _playerInput.actions["Dance"];
    }

    private void Update()
    {
        MoveInput = _move.ReadValue<Vector2>();
        LookInput = _look.ReadValue<Vector2>();
        IsRunning = _run.IsPressed();
        IsDancing = _dance.WasPressedThisFrame();
    }
}
