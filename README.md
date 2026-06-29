# Adminmenu

Minimal Arma Reforger admin-menu mod scaffold.

## Features

- F9 overlay, backed by standard Enfusion widgets.
- Mouse activation plus keyboard navigation through `MenuUp`, `MenuDown`, `MenuLeft`, `MenuRight`, and `MenuSelect`.
- Client password login using `Reforgeradminaccess`.
- Server RPC confirmation and server-side authorization check for every action.
- Player list, kick, ban, unban, teleport, item spawn, and AI spawn flows.

## Structure

- `Scripts/Game/AdminMenu/UI`: overlay and input handling.
- `Scripts/Game/AdminMenu/Auth`: authentication state and future whitelist replacement point.
- `Scripts/Game/AdminMenu/Server`: server RPC handlers and all privileged actions.
- `UI/layouts/AdminMenu`: overlay layout.
- `Configs/System`: F9 input action merged into the standard character input context.
- `ServerConfig.example.json`: dedicated server test config shape.

## Notes

`PlayerManager.KickPlayer()` is used for kick. Ban and unban use `BackendApi.GetBanServiceApi()` with identity IDs, and ban also kicks the target if they are online.

The first authentication version intentionally keeps the password check in `ADM_AdminAuthStore`. Replace `IsAdminCandidate()` and `ValidatePassword()` with `BackendApi.IsListedServerAdmin()`, `BackendApi.GetPlayerIdentityId()`, or a parsed server config whitelist when ready.

Open `addon.gproj` in Enfusion Workbench, let Workbench resave/generated-resource IDs, then pack and add the packed Workshop/local mod to the dedicated server `game.mods` list. The included `ServerConfig.example.json` shows the server-side shape; replace `LOCAL_ADMINMENU` with the actual published/local mod ID.
