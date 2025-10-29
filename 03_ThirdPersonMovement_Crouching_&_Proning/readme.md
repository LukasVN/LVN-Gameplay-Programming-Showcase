# 03 Third Person Movement : Crouching And Proning

Welcome to the Crouching And Proning section of the LVN Gameplay Programming Showcase series.

Building on the [**02 Jumping Module**](https://github.com/LukasVN/LVN-Gameplay-Programming-Showcase/tree/main/02_ThirdPersonMovement_Jumping), this section introduces crouching and proning mechanics, allowing the player to dynamically adjust their stance based on environment and input.

<h2 align="center">Overview</h2>

<p align="center"> <img src="https://github.com/user-attachments/assets/d29826e8-a5e2-4cf5-a2d3-6b4e6ab3087e" width="600px" /> </p>

The system supports both crouch and prone states, with smooth transitions and clearance checks to prevent clipping into geometry. Capsule resizing and mesh alignment are handled carefully to maintain visual consistency 
and avoid physics glitches.

<h3>Key features include:</h3>

- Toggle-based crouch and prone system with capsule resizing

- Customizable Mesh repositioning to match capsule base height

- Clearance checks before standing or crouch-up from prone

- Movement speed adjustments based on stance and direction

- Slope-safe capsule transitions

- Animation-driven transitions with notifies and events


> Note: Unreal uses SetCapsuleHalfHeight(..., true) with sweep-based offset correction to prevent tunneling on slopes. Unity uses CharacterController.height.

> Extra Note: The Crouching and Proning system is implemented in both **Unity (C#)** and **Unreal Engine (Blueprint)**, with Unreal using animation notifies to trigger physics actions and Unity using animation events.
