#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <string>
#include <vector>
#include <sstream>
#include <algorithm>
#include <cmath>

#include "core/rng.hpp"
#include "game/rarity.hpp"
#include "game/weapon.hpp"
#include "game/weapon_base.hpp"
#include "game/affix.hpp"
#include "game/actor.hpp"
#include "game/inventory.hpp"
#include "game/loot_tables.hpp"
#include "game/combat_math.hpp"

using namespace game;

// ---------- UI control IDs ----------
enum {
    ID_BTN_NEXT = 1001,
    ID_BTN_RESET,
    ID_LB_ENEMIES,
    ID_LB_INV,
    ID_BTN_EQUIP,
    ID_BTN_BEST,
    ID_CHK_AUTO,
    ID_EDIT_LOG,
    ID_STATIC_PLAYER
};

struct UIRefs {
    HWND hEnemies{};
    HWND hInventory{};
    HWND hBtnNext{};
    HWND hBtnReset{};
    HWND hBtnEquip{};
    HWND hBtnBest{};
    HWND hChkAuto{};
    HWND hLog{};
    HWND hPlayer{};
};

struct App {
    core::RNG rng{1337};
    LootTables loot = makeDefaultLoot();
    Inventory inv;
    Actor player{ "Player", 60, 60, 1, Weapon{WeaponBase{"Rusty Sword",2,6}, Rarity::Common, {}} };
    std::vector<Actor> enemies;
    int selectedEnemy = 0;
    bool autoEquipBetter = true;
    std::vector<std::string> log;
    UIRefs ui;
};

// --------- helpers ---------
static void push_log(App& app, const std::string& s) {
    if (app.log.size() > 800) app.log.erase(app.log.begin(), app.log.begin()+400);
    app.log.push_back(s);
}

static std::vector<Actor> make_enemies() {
    Weapon goblinW{ WeaponBase{"Shiv",   1, 4}, Rarity::Common, {} };
    Weapon bruteW { WeaponBase{"Club",   3, 7}, Rarity::Common, {} };
    Weapon raiderW{ WeaponBase{"Hatchet",2, 6}, Rarity::Common, {} };
    return {
        Actor{"Goblin", 20, 20, 0, goblinW},
        Actor{"Brute",  35, 35, 1, bruteW },
        Actor{"Raider", 25, 25, 0, raiderW}
    };
}

static bool any_alive(const std::vector<Actor>& es) {
    for (auto& e : es) if (e.alive()) return true;
    return false;
}

static void reset_battle(App& app) {
    if (auto eq = app.inv.equipped()) app.player.weapon = *eq;
    app.player.hp = app.player.maxHP;
    app.enemies = make_enemies();
    app.selectedEnemy = 0;
    push_log(app, "Battle reset.");
}

static void refresh_player_panel(App& app) {
    std::ostringstream os;
    os << "Player  HP " << app.player.hp << "/" << app.player.maxHP
       << "  Armor " << app.player.armor << "\r\n"
       << "Equipped: " << app.player.weapon.label()
       << "  | DPR: " << expectedDPR(app.player.weapon);
    SetWindowTextA(app.ui.hPlayer, os.str().c_str());
}

static void refresh_enemies_list(App& app) {
    SendMessageA(app.ui.hEnemies, LB_RESETCONTENT, 0, 0);
    for (size_t i=0;i<app.enemies.size();++i) {
        const auto& en = app.enemies[i];
        std::ostringstream os;
        os << "[" << i << "] " << en.name
           << (static_cast<int>(i)==app.selectedEnemy ? "  <target>" : "")
           << "  HP " << std::max(0,en.hp) << "/" << en.maxHP
           << "  Armor " << en.armor
           << (en.alive()? "" : "  (dead)");
        SendMessageA(app.ui.hEnemies, LB_ADDSTRING, 0, (LPARAM)os.str().c_str());
    }
    SendMessageA(app.ui.hEnemies, LB_SETCURSEL, app.selectedEnemy, 0);
}

static void refresh_inventory(App& app) {
    SendMessageA(app.ui.hInventory, LB_RESETCONTENT, 0, 0);
    for (size_t i=0;i<app.inv.size();++i) {
        const auto& w = app.inv.at(i);
        bool eq = (app.inv.equippedIndex() == i);
        std::ostringstream os;
        os << "[" << (i<10?"0":"") << i << "] " << (eq?"* ":"  ") << w.label()
           << "  | DPR: " << expectedDPR(w);
        SendMessageA(app.ui.hInventory, LB_ADDSTRING, 0, (LPARAM)os.str().c_str());
    }
}

static void refresh_log(App& app) {
    std::ostringstream os;
    for (auto& s: app.log) { os << s << "\r\n"; }
    SendMessageA(app.ui.hLog, WM_SETREDRAW, FALSE, 0);
    SetWindowTextA(app.ui.hLog, os.str().c_str());
    SendMessageA(app.ui.hLog, EM_SETSEL, (WPARAM)-1, (LPARAM)-1);
    SendMessageA(app.ui.hLog, EM_SCROLLCARET, 0, 0);
    SendMessageA(app.ui.hLog, WM_SETREDRAW, TRUE, 0);
    InvalidateRect(app.ui.hLog, nullptr, TRUE);
}

static void full_refresh(App& app) {
    refresh_player_panel(app);
    refresh_enemies_list(app);
    refresh_inventory(app);
    refresh_log(app);
}

static void do_player_turn(App& app) {
    // pick target: selected if alive else first alive
    auto it = app.enemies.end();
    if (app.selectedEnemy >= 0 && app.selectedEnemy < (int)app.enemies.size() && app.enemies[app.selectedEnemy].alive())
        it = app.enemies.begin()+app.selectedEnemy;
    else
        it = std::find_if(app.enemies.begin(), app.enemies.end(), [](const Actor& e){return e.alive();});
    if (it == app.enemies.end()) return;

    Actor& target = *it;
    int hits = std::max(1, (int)std::round(app.player.weapon.attackSpeed()));
    for (int h=0; h<hits && target.alive(); ++h) {
        int dmg = app.player.attack(target, app.rng);
        target.hp -= dmg;
        std::ostringstream os;
        os << "You hit " << target.name << " for " << dmg
           << "  (" << std::max(0,target.hp) << "/" << target.maxHP << ")";
        push_log(app, os.str());
    }
    if (!target.alive()) {
        std::ostringstream os; os << target.name << " is slain!";
        push_log(app, os.str());
        Weapon drop = app.loot.rollWeapon(app.rng, 1);
        push_log(app, std::string("Loot dropped: ") + drop.label());
        size_t idx = app.inv.add(drop);
        if (app.autoEquipBetter) {
            double cur = expectedDPR(app.player.weapon);
            double cand = expectedDPR(app.inv.at(idx));
            if (cand > cur) {
                app.inv.equip(idx);
                app.player.weapon = *app.inv.equipped();
                std::ostringstream os2;
                os2 << "Auto-equipped better weapon (" << cand << " DPR > " << cur << " DPR).";
                push_log(app, os2.str());
            }
        }
    }
}

static void do_enemies_turn(App& app) {
    for (auto& e : app.enemies) {
        if (!e.alive() || !app.player.alive()) continue;
        int hits = std::max(1, (int)std::round(e.weapon.attackSpeed()));
        for (int h=0; h<hits && app.player.alive(); ++h) {
            int dmg = e.attack(app.player, app.rng);
            app.player.hp -= dmg;
            std::ostringstream os;
            os << e.name << " hits you for " << dmg
               << "  (You: " << std::max(0,app.player.hp) << "/" << app.player.maxHP << ")";
            push_log(app, os.str());
        }
    }
}

// ---------- window layout ----------
static void layout_controls(HWND hWnd, UIRefs& ui) {
    RECT rc; GetClientRect(hWnd, &rc);
    int w = rc.right, h = rc.bottom;

    int pad = 8;
    int leftW = w * 2 / 5;           // left pane for enemies & player
    int rightW = w - leftW - pad*3;  // right pane for inventory & log
    int col1X = pad, col2X = leftW + pad*2;

    int y = pad;
    // Player panel (static)
    MoveWindow(ui.hPlayer, col1X, y, leftW - pad, 60, TRUE);
    y += 60 + pad;

    // Enemies list
    MoveWindow(ui.hEnemies, col1X, y, leftW - pad, h - y - 2*pad - 30, TRUE);
    // Buttons below enemies
    MoveWindow(ui.hBtnNext,  col1X, h - pad - 30, 110, 30, TRUE);
    MoveWindow(ui.hBtnReset, col1X + 120, h - pad - 30, 110, 30, TRUE);

    // Right column
    int y2 = pad;
    MoveWindow(ui.hChkAuto,  col2X, y2, 180, 24, TRUE);
    MoveWindow(ui.hBtnBest,  col2X + 190, y2, 120, 24, TRUE);
    MoveWindow(ui.hBtnEquip, col2X + 320, y2, 120, 24, TRUE);
    y2 += 24 + pad;

    int invH = (h - y2 - pad*2) / 2;
    MoveWindow(ui.hInventory, col2X, y2, rightW, invH, TRUE);
    y2 += invH + pad;
    MoveWindow(ui.hLog, col2X, y2, rightW, h - y2 - pad, TRUE);
}

// ---------- Win32 plumbing ----------
static App* g_app = nullptr;

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CREATE: {
            g_app = new App();

            // starter inventory
            size_t starterIdx = g_app->inv.add(Weapon{WeaponBase{"Rusty Sword",2,6}, Rarity::Common, {}});
            g_app->inv.equip(starterIdx);
            if (auto eq = g_app->inv.equipped()) g_app->player.weapon = *eq;
            g_app->enemies = make_enemies();
            push_log(*g_app, "Welcome! Click Next Round to fight. Select an enemy on the left.");
            // Create controls (ANSI versions to avoid wide conversions)
            HFONT hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);

            g_app->ui.hPlayer = CreateWindowExA(0, "STATIC", "",
                WS_CHILD|WS_VISIBLE|SS_LEFT, 0,0,0,0, hWnd, (HMENU)ID_STATIC_PLAYER, GetModuleHandle(nullptr), nullptr);
            SendMessage(g_app->ui.hPlayer, WM_SETFONT, (WPARAM)hFont, TRUE);

            g_app->ui.hEnemies = CreateWindowExA(WS_EX_CLIENTEDGE, "LISTBOX", "",
                WS_CHILD|WS_VISIBLE|LBS_NOTIFY|WS_VSCROLL, 0,0,0,0, hWnd, (HMENU)ID_LB_ENEMIES, GetModuleHandle(nullptr), nullptr);
            SendMessage(g_app->ui.hEnemies, WM_SETFONT, (WPARAM)hFont, TRUE);

            g_app->ui.hBtnNext = CreateWindowExA(0, "BUTTON", "Next Round",
                WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON, 0,0,0,0, hWnd, (HMENU)ID_BTN_NEXT, GetModuleHandle(nullptr), nullptr);
            SendMessage(g_app->ui.hBtnNext, WM_SETFONT, (WPARAM)hFont, TRUE);

            g_app->ui.hBtnReset = CreateWindowExA(0, "BUTTON", "Reset Battle",
                WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON, 0,0,0,0, hWnd, (HMENU)ID_BTN_RESET, GetModuleHandle(nullptr), nullptr);
            SendMessage(g_app->ui.hBtnReset, WM_SETFONT, (WPARAM)hFont, TRUE);

            g_app->ui.hChkAuto = CreateWindowExA(0, "BUTTON", "Auto-equip on drop",
                WS_CHILD|WS_VISIBLE|BS_AUTOCHECKBOX, 0,0,0,0, hWnd, (HMENU)ID_CHK_AUTO, GetModuleHandle(nullptr), nullptr);
            SendMessage(g_app->ui.hChkAuto, WM_SETFONT, (WPARAM)hFont, TRUE);
            SendMessage(g_app->ui.hChkAuto, BM_SETCHECK, BST_CHECKED, 0);

            g_app->ui.hBtnBest = CreateWindowExA(0, "BUTTON", "Equip Best",
                WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON, 0,0,0,0, hWnd, (HMENU)ID_BTN_BEST, GetModuleHandle(nullptr), nullptr);
            SendMessage(g_app->ui.hBtnBest, WM_SETFONT, (WPARAM)hFont, TRUE);

            g_app->ui.hBtnEquip = CreateWindowExA(0, "BUTTON", "Equip Selected",
                WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON, 0,0,0,0, hWnd, (HMENU)ID_BTN_EQUIP, GetModuleHandle(nullptr), nullptr);
            SendMessage(g_app->ui.hBtnEquip, WM_SETFONT, (WPARAM)hFont, TRUE);

            g_app->ui.hInventory = CreateWindowExA(WS_EX_CLIENTEDGE, "LISTBOX", "",
                WS_CHILD|WS_VISIBLE|LBS_NOTIFY|WS_VSCROLL, 0,0,0,0, hWnd, (HMENU)ID_LB_INV, GetModuleHandle(nullptr), nullptr);
            SendMessage(g_app->ui.hInventory, WM_SETFONT, (WPARAM)hFont, TRUE);

            g_app->ui.hLog = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", "",
                WS_CHILD|WS_VISIBLE|ES_MULTILINE|ES_READONLY|WS_VSCROLL|ES_AUTOVSCROLL,
                0,0,0,0, hWnd, (HMENU)ID_EDIT_LOG, GetModuleHandle(nullptr), nullptr);
            SendMessage(g_app->ui.hLog, WM_SETFONT, (WPARAM)hFont, TRUE);
            SendMessageA(g_app->ui.hLog, EM_SETLIMITTEXT, 1024*1024, 0);

            layout_controls(hWnd, g_app->ui);
            full_refresh(*g_app);
            return 0;
        }
        case WM_SIZE:
            if (g_app) { layout_controls(hWnd, g_app->ui); }
            return 0;

        case WM_COMMAND: {
            if (!g_app) break;
            int id = LOWORD(wParam);
            int code = HIWORD(wParam);

            if (id == ID_LB_ENEMIES && code == LBN_SELCHANGE) {
                int sel = (int)SendMessage(g_app->ui.hEnemies, LB_GETCURSEL, 0, 0);
                if (sel >= 0) g_app->selectedEnemy = sel;
                refresh_enemies_list(*g_app);
                return 0;
            }
            if (id == ID_LB_INV && code == LBN_SELCHANGE) {
                // no-op; equip on button
                return 0;
            }
            if (id == ID_CHK_AUTO && code == BN_CLICKED) {
                LRESULT state = SendMessage(g_app->ui.hChkAuto, BM_GETCHECK, 0, 0);
                g_app->autoEquipBetter = (state == BST_CHECKED);
                return 0;
            }
            if (id == ID_BTN_BEST && code == BN_CLICKED) {
                if (g_app->inv.equipBest()) {
                    g_app->player.weapon = *g_app->inv.equipped();
                    push_log(*g_app, "Equipped best-by-DPR.");
                    refresh_player_panel(*g_app);
                    refresh_inventory(*g_app);
                    refresh_log(*g_app);
                }
                return 0;
            }
            if (id == ID_BTN_EQUIP && code == BN_CLICKED) {
                int sel = (int)SendMessage(g_app->ui.hInventory, LB_GETCURSEL, 0, 0);
                if (sel >= 0 && sel < (int)g_app->inv.size()) {
                    g_app->inv.equip((size_t)sel);
                    g_app->player.weapon = *g_app->inv.equipped();
                    std::ostringstream os; os << "Equipped: " << g_app->inv.at((size_t)sel).label();
                    push_log(*g_app, os.str());
                    refresh_player_panel(*g_app);
                    refresh_inventory(*g_app);
                    refresh_log(*g_app);
                }
                return 0;
            }
            if (id == ID_BTN_RESET && code == BN_CLICKED) {
                reset_battle(*g_app);
                full_refresh(*g_app);
                return 0;
            }
            if (id == ID_BTN_NEXT && code == BN_CLICKED) {
                if (!g_app->player.alive()) { push_log(*g_app, "You are dead. Reset to continue."); refresh_log(*g_app); return 0; }
                if (!any_alive(g_app->enemies)) { push_log(*g_app, "No enemies alive. Reset to continue."); refresh_log(*g_app); return 0; }
                do_player_turn(*g_app);
                do_enemies_turn(*g_app);
                if (!g_app->player.alive()) push_log(*g_app, "Defeat. You died.");
                else if (!any_alive(g_app->enemies)) push_log(*g_app, "Victory! All enemies defeated.");
                full_refresh(*g_app);
                return 0;
            }
            break;
        }

        case WM_DESTROY:
            if (g_app) { delete g_app; g_app = nullptr; }
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE, LPSTR, int nCmdShow) {
    const char* cls = "CastleCombatWnd";
    WNDCLASSA wc = {};
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInst;
    wc.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
    wc.lpszClassName = cls;
    RegisterClassA(&wc);

    HWND hWnd = CreateWindowExA(0, cls, "Castle-like Combat (Win32 Prototype)",
        WS_OVERLAPPEDWINDOW|WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT, 1200, 720,
        nullptr, nullptr, hInst, nullptr);

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return (int)msg.wParam;
}
