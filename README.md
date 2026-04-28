# Space Station Escape

Space Station Escape is a 3D puzzle escape game built with C++ and OpenGL. The player is trapped inside a damaged space station after a meteor impact and must repair critical systems before escaping.

The game focuses on exploring connected rooms, solving station repair puzzles, and using environmental clues to unlock the final control room.

## Itch.io Links

**TODO: add itch.io project link**

## Screenshots

TODO: Add screenshots here.

```md
![Main room](screenshots/main-room.png)
![Oxygen puzzle](screenshots/oxygen-puzzle.png)
![Power puzzle](screenshots/power-puzzle.png)
```

## Demo Video

**TODO: add demo vdo**

## Gameplay

After a meteor strike damages the station, the emergency AI guides the player through the repair sequence:

1. Fix the oxygen system by activating the valves in the correct order.
2. Restore main power by solving the wire-cutting puzzle.
3. Recover a clue from storage.
4. Decode the clue in the lab puzzle.
5. Unlock the control room and authorize the escape route.

The player wins by authorizing the escape route from the control terminal. The player can lose by failing critical repair puzzles.

## Controls

- `WASD` - Move
- `Mouse` - Move camera
- `Shift` - Run
- `E` - Interact
- `Enter` - Confirm puzzle input
- `Arrow keys` - Navigate or adjust puzzle values
- `Backspace` - Delete code input
- `Esc` - Close puzzle HUD or exit
- `R` - Restart after winning or losing

## Features

- Third-person 3D exploration
- Room-based space station layout
- Oxygen, power, lab, and code puzzles
- Collision-aware player and camera movement
- HUD panels for puzzle interactions
- Story subtitles and objective guidance
- Sound effects and background audio using miniaudio
- Animated player character and animated oxygen pipe sequence

## Build

This project uses CMake.

```powershell
cmake --preset x64-release
cmake --build out/build/x64-release --config Release
```

The executable is generated at:

```text
out/build/x64-release/SpaceStationEscape.exe
```

For a playable release package, place the executable next to the runtime asset folders:

```text
SpaceStationEscape-release/
  SpaceStationEscape.exe
  assets/
  shaders/
```

## Source Code

Main source folders:

- `src/` - C++ game code
- `src/audio/` - Audio playback system
- `src/graphics/` - Shader, texture, model, and animation rendering helpers
- `src/world/` - Space station layout, interactables, collisions, and scene placement
- `shaders/` - GLSL shader files
- `include/` - Header-only third-party libraries

## Attribution

TODO: Fill in exact asset/source links before submission.

- OpenGL - Graphics API
- GLFW - Window creation and input
- GLAD - OpenGL function loader
- GLM - Mathematics library
- Assimp - Model and animation loading
- miniaudio - Audio playback
- 3D character model and animations: **TODO: add source/author/license**
- Space station models and props: **TODO: add source/author/license**
- Puzzle posters/textures: **TODO: add source/author/license**
- Sound effects and background audio: **TODO: add source/author/license**
