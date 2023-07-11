// SPDX-FileCopyrightText: 2023 Martin J. Fiedler <keyj@emphy.de>
// SPDX-License-Identifier: MIT

#pragma once

enum class AppEvent {
    Left, Right, Up, Down,
    PageUp, PageDown, Home, End,
    A, B, X, Y,
    RS, LS, RT, LT,
    Select, Start, Logo
};

enum class AppAction {
    Quit,
    Minimize,
    Restore
};
