# SDK Launcher

An SDK launcher for Strata Source engine games.

To install, download the latest release for the given game and extract
the executable to `bin/win64` or `bin/linux64` (depending on your OS).

### Default Config

If you would like to modify the config loaded at startup, create a config next to
the SDK launcher executable called `SDKLauncherDefault.json`. The SDK launcher will
load this config file instead of the internal config. Alternatively, it will
automatically load the last loaded config file, so you can load your config manually
once and it will load that config on subsequent launches.

### Config Format

Here is an example config file that may be loaded into the SDK launcher.

**Comments are added for clarity, but they are NOT allowed in actual configs!**

```json5
{
  // The name of the game directory (if relative), or an absolute path. If this field is an absolute path,
  // the game_icon field (which assumes ${GAME} is relative) should be updated to prevent a missing logo.
  // Use ${SOURCEMODS} to enter the SourceMods folder, for example "${SOURCEMODS}/mymod"
  "game_default": "p2ce",
  // Optional, the default is "${ROOT}/${GAME}/resource/game.ico"
  "game_icon": "${ROOT}/${GAME}/resource/game.ico",
  // Optional, the default is false (set this to true if the SDK launcher is inside bin/ instead of bin/${PLATFORM}/)
  "uses_legacy_bin_dir": false,
  // Optional, the default is 450 (changes the default height of the window)
  "window_height": 450,
  // Optional, holds the download URL of the mod template for the game (must point to a zip file)
  // For reference, this is the P2CE template mod download URL:
  "mod_template_url": "https://github.com/StrataSource/p2ce-mod-template/archive/refs/heads/main.zip",
  // Optional, the default is false (enables P2CE-style addons)
  "supports_p2ce_addons": false,
  // Sections hold titled groups of buttons
  "sections": [
    {
      // Each section has a title...
      "name": "Commands",
      // ...and a group of buttons.
      "entries": [
        {
          // Every button must have a name...
          "name": "Portal 2: CE - Dev Mode",
          // ...a type ("command", "link", or "directory")...
          "type": "command",
          // ...and an action. ${ROOT} expands to the root path, and ${PLATFORM}
          // expands to win64 or linux64 depending on the platform. If the type
          // of the action is command, ".exe" will be appended to search for the
          // default icon on Windows. Link types are Internet URLs, and directory
          // types are directories that open in a file explorer.
          "action": "${ROOT}/bin/${PLATFORM}/strata", // Expands to "<Game Directory>/bin/win64/strata" on Windows.
          // Arguments are optional for command-type actions, and will be passed
          // to the command when ran. ${GAME} expands to the game directory name.
          "arguments": ["-game", "${GAME}", "-dev"],
          // Optional, the icon override allows the config creator to change the
          // default icon for any button. ${STRATA_ICON} is a special keyword that
          // picks the appropriate Strata icon based on the theme of the application.
          // ${SDKLAUNCHER_ICON} places the icon of the SDK Launcher application,
          // and will fall back to the Strata icon if there is no custom icon for
          // the current compiled game. Filesystem paths are supported here.
          "icon_override": "${STRATA_ICON}",
          // Optional, allows buttons to only appear on a given OS. Allowed values
          // are "windows", "linux", or both in the same string like "windows,linux"
          // (although that's equivalent to not adding this value in the first place).
          "os": "windows"
        },
        {
          // & needs to be escaped with another & - this will appear as one &.
          // Thank Qt for that!
          "name": "Portal 2: CE - Dev Mode && Tools Mode",
          "type": "command",
          "action": "${ROOT}/bin/${PLATFORM}/strata",
          "arguments": ["-game", "${GAME}", "-dev", "-tools"],
          "icon_override": "${GAME_ICON}"
        }
      ]
    },
    {
      "name": "Links",
      "entries": [
        {
          "name": "Strata Source Issue Tracker",
          "type": "link",
          "action": "https://github.com/StrataSource/Engine/issues",
          "icon_override": "${STRATA_ICON}"
        }
      ]
    },
    {
      "name": "Folders",
      "entries": [
        {
          "name": "Open Root Folder",
          "type": "directory",
          "action": "${ROOT}"
        }
      ]
    }
  ]
}
```
