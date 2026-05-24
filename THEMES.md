# Creating Custom Themes for SPECTRUM

You can create custom themes for SPECTRUM by adding `.json` files to the `themes/` directory located next to the executable.

## JSON Schema Example

Create a file (e.g., `my_theme.json`) with the following structure:

```json
{
    "theme.name": "Midnight Neon",
    "theme.id": "midnight-neon",
    "theme.properties": {
        "spectrum.tui.colorR": 0.0,
        "spectrum.tui.colorG": 1.0,
        "spectrum.tui.colorB": 0.8,
        "spectrum.tui.useRandomCharacters": false,
        "spectrum.tui.customCharacter": "#",
        "spectrum.tui.visualizerMode": "Default Mode",
        "spectrum.tui.key": 51
    }
}
```

## Property Definitions

| Property | Type | Range | Description |
| :--- | :--- | :--- | :--- |
| `theme.name` | String | Max 64 chars | The display name of your theme. |
| `theme.id` | String | Max 64 chars | A unique internal identifier for the theme. |
| `spectrum.tui.colorR` | Double | `0.0` to `1.0` | Red intensity (0 is none, 1.0 is max). |
| `spectrum.tui.colorG` | Double | `0.0` to `1.0` | Green intensity. |
| `spectrum.tui.colorB` | Double | `0.0` to `1.0` | Blue intensity. |
| `spectrum.tui.useRandomCharacters` | Boolean | `true/false` | If true, bars use random musical/ASCII symbols. |
| `spectrum.tui.customCharacter` | String | 1 character | The character used to draw bars if `useRandomCharacters` is false. |
| `spectrum.tui.visualizerMode` | String | Enum | See **Visualizer Modes** below. |
| `spectrum.tui.key` | Integer | 0-254 | Windows Virtual-Key code to switch to this theme at runtime. |

## Visualizer Modes

Use these exact strings in the `spectrum.tui.visualizerMode` property:

*   **`Default Mode`**: Standard bottom-up bars.
*   **`Mode 1`**: Centered bars expanding outward.
*   **`Mode 2`**: Floating "needle" mode.
*   **`Mode 3`**: Inverted centered bars.
*   **`Mode 4`**: Sparkle/Intensity mode (height controls brightness).

## Common Hotkeys (spectrum.tui.key)

| Key | Value |
| :--- | :--- |
| **3** | `51` |
| **4** | `52` |
| **F3** | `114` |
| **F4** | `115` |

*Note: Keys **1** (49) and **2** (50) are reserved for the built-in Pink and Gradient themes.*

## Possible Errors

1.  **JSON Syntax Error**: Ensure your commas and quotes are correct. If the JSON is invalid, SPECTRUM will skip the file.
2.  **Missing Keys**: If a property is missing, the theme will use the built-in default values.
3.  **Color Range**: Values outside `0.0` - `1.0` will be clamped.
4.  **Key Conflicts**: If two custom themes use the same hotkey, the first one loaded alphabetically will win.
5.  **File Location**: Themes **must** be inside a folder named `themes` in the same directory as `spectrum.exe`.
