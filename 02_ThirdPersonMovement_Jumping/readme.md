# 02 Third Person Movement : Jumping

Welcome to the Jumping section of the LVN Gameplay Programming Showcase series.

Building on the [**01 Base Mobility**](https://github.com/LukasVN/LVN-Gameplay-Programming-Showcase/tree/main/01_ThirdPersonMovement_BaseMobility) foundation, this section adds vertical movement and mid-air control to the core movement logic.

<h2 align="center">Overview</h2>

<p align="center">
  <img src="https://github.com/user-attachments/assets/a11dadd7-629c-49c7-b442-c031fd8ce431" width="600px" />
</p>

The system uses animation-driven jumping, where jump and flip (double jump) forces are triggered by keyframe events in the animations. This ensures smooth coordination between gameplay and animation, keeping visual timing consistent, even if the responsiveness differs slightly from purely code-driven jumps.

<h3>Key features include:</h3>

- Jump input buffering for more responsive controls

- Mid-air flip (double jump) if enabled

- Flip can be triggered while falling without a previous jump

- Jump and flip forces triggered via animation notifies (Unreal) or animation events (Unity)

- Smooth state transitions between jump, fall, flip, and landing

- Velocity reset before flip to maintain consistent launch height

- Fully synchronized with Mixamo animations

- Customizable parameters including jump force, flip force, buffer timing, gravity scale (Unreal), air control (Unreal), ground detection (Unity), and double jump toggle

> Note: The jump system is implemented in both **Unity (C#)** and **Unreal Engine (Blueprint)**, with Unreal using animation notifies to trigger physics actions and Unity using animation events.
