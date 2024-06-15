# SDK Launcher

A minimal SDK launcher for Strata Source engine games.

To install, download the latest release for the given game and extract
the executable to `bin/win64` or `bin/linux64` (depending on your OS).

### Default Config

If you are working on a mod and would like to use this SDK launcher for your mod,
you can create a config called `sdk_launcher_default.json` and place it next to the
SDK launcher executable. It will load this config file when the config history is
clean, so anyone running this application for the first time will load your mod's
provided config immediately.

### Config Format

Here is an example config file that may be imported into the SDK launcher.

**Comments are added for clarity, but they are NOT allowed in actual configs!**

```json5
{
  // Type can be "steam" or "custom".
  "type": "steam",
  // If the type is "steam", it will look for the AppId of the app here,
  // as well as automatically fill out the root path.
  "appid": "440000",
  // If the type is "custom", rather than have an AppId, it just has the
  // path to the root of the game/mod here. This path is allowed to be relative.
  "root": ".",
  // To be clear, if the type is steam, appid is required but root should not be
  // present. If the type is custom, appid should not be present, but root should be.
  // You will most likely use the custom type.

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
          "action": "${ROOT}/bin/${PLATFORM}/strata", // Expands to "./bin/win64/strata" on Windows.
          // Arguments are optional for command-type actions, and will be passed
          // to the command when ran.
          "arguments": ["-game", "p2ce", "-dev"],
          // The icon override (optional) allows the config creator to change the
          // icon for any button. ${GAME_ICON} is a special keyword for steam-type
          // configs which loads the game icon from Steam. ${STRATA_ICON} is a
          // special keyword that picks the appropriate Strata icon based on the
          // theme of the application. Filesystem paths are supported here.
          "icon_override": "${GAME_ICON}",
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
          "arguments": ["-game", "p2ce", "-dev", "-tools"],
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
