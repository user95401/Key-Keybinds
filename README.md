# Key Keybinds

This mod allows you to create new "key emulation" bindable actions.

For example you can add the "Shit+F12" action and set <cy>`E`</c> keybind for it. And now if you press <cy>`E`</c>, game gets <cy>`Shit+F12`</c> also.

Using `PressBindEvent` posting (from Custom Keybinds) and `CCKeyboardDispatcher::dispatchKeyboardMSG` from game.

*Supports  `ControllerBind` and  `Keybind` binds only.*