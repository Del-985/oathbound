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
#include "game/slots.hpp"
#include "game/gear.hpp"
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
    ID_LB_INVW,          // weapons
    ID_LB_INVG,          // gear
    ID_BTN_EQ_MAIN,
    ID_BTN_EQ_OFF,
    ID_BTN_EQ_GEAR,
    ID_BTN_BEST,
    ID_CHK_AUTO,
    ID_EDIT_LOG,
    ID_STATIC_PLAYER,
    ID_STATIC_EQUIP
};

struct UIRefs {
    HWND hEnemies{}, hInvW{}, hInvG{};
    HWND hBtnNext{}, hBtnReset{}, hBtnEqMain{}, hBtnEqOff{}, hBtnEqGear{}, hBtnBest{};
    HWND hChkAuto{}, hLog{}, hPlayer{}, hEquip{};
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

// -------- helpers --------
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

static void apply_equipment_to_player(App& app) {
    // Armor from gear + (shield in off-hand if equipped as gear)
    GearBonuses b = app.inv.bonuses();
    app.player.armor = 1 + b.armor; // base 1 + gear
}

static void refresh_player_panel(App& app) {
    GearBonuses b = app.inv.bonuses();
    const Weapon* mw = app.inv.equipped();
    std::ostringstream os;
    os << "Player  HP " << app.player.hp << "/" << app.player.maxHP
       << "  Armor " << app.player.armor << "\r\n";
    if (mw) {
        os << "Main-hand: " << mw->label()
           << "  | DPR: " << expectedDPR(*mw, b.pctDamage, b.critChance, b.attackSpeed);
    } else {
        os << "Main-hand: (none)";
    }
    const Weapon* ow = app.inv.equippedOffhand();
    os << "\r\nOff-hand Weapon: " << (ow ? ow->label() : "(none)");
    SetWindowTextA(app.ui.hPlayer, os.str().c_str());
}

static void refresh_equip_summary(App& app) {
    auto line = [&](Slot s){
        const Gear* g = app.inv.equipped(s);
        std::ostringstream os;
        os << slotName(s) << ": " << (g ? g->label() : "(none)");
        return os.str();
    };
    std::ostringstream os;
    os << line(Slot::Armor)  << "\r\n"
       << line(Slot::Helmet) << "\r\n"
       << line(Slot::Boots)  << "\r\n"
       << line(Slot::Belt)   << "\r\n"
       << line(Slot::Amulet) << "\r\n"
       << line(Slot::Ring1)  << "\r\n"
       << line(Slot::Ring2)  << "\r\n"
       << "Off-hand Shield: " << (app.inv.equipped(Slot::Offhand) ? app.inv.equipped(Slot::Offhand)->label() : "(none)");
    SetWindowTextA(app.ui.hEquip, os.str().c_str());
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

static void refresh_inv_lists(App& app) {
    SendMessageA(app.ui.hInvW, LB_RESETCONTENT, 0, 0);
    for (size_t i=0;i<app.inv.weaponsCount();++i) {
        const auto& w = app.inv.weaponAt(i);
        bool eqMain = (app.inv.eq_.mainHand == i);
        bool eqOff  = (app.inv.eq_.offHandWpn == i);
        std::ostringstream os;
        os << "[" << (i<10?"0":"") << i << "] " << (eqMain?"* ":"  ") << (eqOff?"(off) ":"")
           << w.label();
        SendMessageA(app.ui.hInvW, LB_ADDSTRING, 0, (LPARAM)os.str().c_str());
    }
    SendMessageA(app.ui.hInvG, LB_RESETCONTENT, 0, 0);
    for (size_t i=0;i<app.inv.gearCount();++i) {
        const auto& g = app.inv.gearAt(i);
        std::ostringstream os;
        os << "[" << (i<10?"0":"") << i << "] (" << slotName(g.slot) << ") " << g.label();
        SendMessageA(app.ui.hInvG, LB_ADDSTRING, 0, (LPARAM)os.str().c_str());
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
    apply_equipment_to_player(app);
    refresh_player_panel(app);
    refresh_equip_summary(app);
    refresh_enemies_list(app);
    refresh_inv_lists(app);
    refresh_log(app);
}

static void reset_battle(App& app) {
    if (auto eq = app.inv.equipped()) app.player.weapon = *eq;
    app.player.hp = app.player.maxHP;
    app.enemies = make_enemies();
    app.selectedEnemy = 0;
    push_log(app, "Battle reset.");
}

static void do_player_turn(App& app) {
    GearBonuses b = app.inv.bonuses();
    const Weapon* mw = app.inv.equipped();
    if (!mw) return;

    // pick target
    auto it = app.enemies.end();
    if (app.selectedEnemy >= 0 && app.selectedEnemy < (int)app.enemies.size() && app.enemies[app.selectedEnemy].alive())
        it = app.enemies.begin()+app.selectedEnemy;
    else
        it = std::find_if(app.enemies.begin(), app.enemies.end(), [](const Actor& e){return e.alive();});
    if (it == app.enemies.end()) return;

    Actor& target = *it;

    // Main-hand attacks
    int hits = std::max(1, (int)std::round(expectedAPS(*mw, b.attackSpeed)));
    for (int h=0; h<hits && target.alive(); ++h) {
        int dmg = app.player.attack(target, app.rng, b.pctDamage, b.critChance);
        target.hp -= dmg;
        std::ostringstream os;
        os << "You (MH) hit " << target.name << " for " << dmg
           << "  (" << std::max(0,target.hp) << "/" << target.maxHP << ")";
        push_log(app, os.str());
    }

    // Off-hand weapon (one swing per round for simplicity)
    if (target.alive()) {
        if (const Weapon* off = app.inv.equippedOffhand()) {
            int dmg = rollDamageWithBonuses(*off, app.rng, b.pctDamage, b.critChance);
            dmg = std::max(0, dmg - target.armor);
            target.hp -= dmg;
            std::ostringstream os; os << "You (OH) hit " << target.name << " for " << dmg
                                      << "  (" << std::max(0,target.hp) << "/" << target.maxHP << ")";
            push_log(app, os.str());
        }
    }

    if (!target.alive()) {
        std::ostringstream os; os << target.name << " is slain!";
        push_log(app, os.str());

        // Drop: keep weapons for now (gear can be added later)
        Weapon drop = app.loot.rollWeapon(app.rng, 1);
        size_t idx = app.inv.addWeapon(drop);
        push_log(app, std::string("Loot dropped (weapon): ") + drop.label());

        if (app.autoEquipBetter) {
            double cur = expectedDPR(*mw, b.pctDamage, b.critChance, b.attackSpeed);
            double cand = expectedDPR(app.inv.weaponAt(idx), b.pctDamage, b.critChance, b.attackSpeed);
            if (cand > cur) {
                app.inv.equip(idx);
                app.player.weapon = app.inv.weaponAt(idx);
                std::ostringstream os2; os2 << "Auto-equipped better weapon (" << cand << " DPR > " << cur << " DPR).";
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
            int dmg = e.attack(app.player, app.rng); // enemies have no gear bonuses
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
    int leftW = w * 2 / 5;           // left pane
    int rightW = w - leftW - pad*3;  // right pane
    int col1X = pad, col2X = leftW + pad*2;

    int y = pad;
    MoveWindow(ui.hPlayer, col1X, y, leftW - pad, 80, TRUE);
    y += 80 + pad;

    MoveWindow(ui.hEquip, col1X, y, leftW - pad, h - y - 2*pad - 30 - 160, TRUE);
    y = h - pad - 30 - 160;

    MoveWindow(ui.hEnemies, col1X, y, leftW - pad, 160, TRUE);
    MoveWindow(ui.hBtnNext,  col1X, h - pad - 30, 110, 30, TRUE);
    MoveWindow(ui.hBtnReset, col1X + 120, h - pad - 30, 110, 30, TRUE);

    int y2 = pad;
    MoveWindow(ui.hChkAuto,  col2X, y2, 180, 24, TRUE);
    MoveWindow(ui.hBtnBest,  col2X + 190, y2, 120, 24, TRUE);
    y2 += 24 + pad;

    MoveWindow(ui.hInvW, col2X, y2, rightW, 180, TRUE);
    y2 += 180 + pad;
    MoveWindow(ui.hBtnEqMain, col2X, y2, 120, 26, TRUE);
    MoveWindow(ui.hBtnEqOff,  col2X + 130, y2, 140, 26, TRUE);
    y2 += 26 + pad;

    MoveWindow(ui.hInvG, col2X, y2, rightW, 180, TRUE);
    y2 += 180 + pad;
    MoveWindow(ui.hBtnEqGear, col2X, y2, 140, 26, TRUE);
    y2 += 26 + pad;

    MoveWindow(ui.hLog, col2X, y2, rightW, h - y2 - pad, TRUE);
}

// ---------- Win32 plumbing ----------
static App* g_app = nullptr;

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CREATE: {
            g_app = new App();

            // starter inventory (weapons)
            size_t starterIdx = g_app->inv.addWeapon(Weapon{WeaponBase{"Rusty Sword",2,6}, Rarity::Common, {}});
            g_app->inv.equip(starterIdx);
            g_app->player.weapon = g_app->inv.weaponAt(starterIdx);

            // starter gear
            g_app->inv.add(Gear{"Wooden Shield", Slot::Offhand, Rarity::Common, 2, {}});
            g_app->inv.add(Gear{"Leather Armor", Slot::Armor,  Rarity::Common, 3, {}});
            g_app->inv.add(Gear{"Cloth Hood",    Slot::Helmet, Rarity::Common, 1, {}});
            g_app->inv.add(Gear{"Traveler's Boots", Slot::Boots, Rarity::Common, 1, {}});
            g_app->inv.add(Gear{"Belt of Haste", Slot::Belt, Rarity::Magic, 0, { Affix::Suffix("of Haste",0,0,0.0,0.0,0.15) }});
            g_app->inv.add(Gear{"Amulet of Precision", Slot::Amulet, Rarity::Magic, 0, { Affix::Prefix("Keen",0,0,0.0,0.05,0.0) }});
            g_app->inv.add(Gear{"Copper Ring", Slot::Ring1, Rarity::Magic, 0, { Affix::Suffix("of Embers",0,0,0.08,0.0,0.0) }});
            g_app->inv.add(Gear{"Silver Ring", Slot::Ring1, Rarity::Magic, 0, { Affix::Prefix("Swift",0,0,0.0,0.0,0.10) }});

            g_app->enemies = make_enemies();
            push_log(*g_app, "Welcome! Select target, equip items, and click Next Round.");

            HFONT hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);

            g_app->ui.hPlayer = CreateWindowExA(0, "STATIC", "",
                WS_CHILD|WS_VISIBLE|SS_LEFT, 0,0,0,0, hWnd, (HMENU)ID_STATIC_PLAYER, GetModuleHandle(nullptr), nullptr);
            SendMessage(g_app->ui.hPlayer, WM_SETFONT, (WPARAM)hFont, TRUE);

            g_app->ui.hEquip = CreateWindowExA(0, "STATIC", "",
                WS_CHILD|WS_VISIBLE|SS_LEFT, 0,0,0,0, hWnd, (HMENU)ID_STATIC_EQUIP, GetModuleHandle(nullptr), nullptr);
            SendMessage(g_app->ui.hEquip, WM_SETFONT, (WPARAM)hFont, TRUE);

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

            g_app->ui.hInvW = CreateWindowExA(WS_EX_CLIENTEDGE, "LISTBOX", "",
                WS_CHILD|WS_VISIBLE|LBS_NOTIFY|WS_VSCROLL, 0,0,0,0, hWnd, (HMENU)ID_LB_INVW, GetModuleHandle(nullptr), nullptr);
            SendMessage(g_app->ui.hInvW, WM_SETFONT, (WPARAM)hFont, TRUE);

            g_app->ui.hBtnEqMain = CreateWindowExA(0, "BUTTON", "Equip Weapon",
                WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON, 0,0,0,0, hWnd, (HMENU)ID_BTN_EQ_MAIN, GetModuleHandle(nullptr), nullptr);
            SendMessage(g_app->ui.hBtnEqMain, WM_SETFONT, (WPARAM)hFont, TRUE);

            g_app->ui.hBtnEqOff = CreateWindowExA(0, "BUTTON", "Equip Off-hand",
                WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON, 0,0,0,0, hWnd, (HMENU)ID_BTN_EQ_OFF, GetModuleHandle(nullptr), nullptr);
            SendMessage(g_app->ui.hBtnEqOff, WM_SETFONT, (WPARAM)hFont, TRUE);

            g_app->ui.hInvG = CreateWindowExA(WS_EX_CLIENTEDGE, "LISTBOX", "",
                WS_CHILD|WS_VISIBLE|LBS_NOTIFY|WS_VSCROLL, 0,0,0,0, hWnd, (HMENU)ID_LB_INVG, GetModuleHandle(nullptr), nullptr);
            SendMessage(g_app->ui.hInvG, WM_SETFONT, (WPARAM)hFont, TRUE);

            g_app->ui.hBtnEqGear = CreateWindowExA(0, "BUTTON", "Equip Gear",
                WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON, 0,0,0,0, hWnd, (HMENU)ID_BTN_EQ_GEAR, GetModuleHandle(nullptr), nullptr);
            SendMessage(g_app->ui.hBtnEqGear, WM_SETFONT, (WPARAM)hFont, TRUE);

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

            if (id == ID_BTN_BEST && code == BN_CLICKED) {
                if (g_app->inv.equipBest()) {
                    if (auto eq = g_app->inv.equipped()) g_app->player.weapon = *eq;
                    push_log(*g_app, "Equipped best-by-DPR.");
                    full_refresh(*g_app);
                }
                return 0;
            }

            if (id == ID_BTN_EQ_MAIN && code == BN_CLICKED) {
                int sel = (int)SendMessage(g_app->ui.hInvW, LB_GETCURSEL, 0, 0);
                if (sel >= 0 && (size_t)sel < g_app->inv.weaponsCount()) {
                    g_app->inv.equip((size_t)sel);
                    g_app->player.weapon = g_app->inv.weaponAt((size_t)sel);
                    push_log(*g_app, "Equipped main-hand weapon.");
                    full_refresh(*g_app);
                }
                return 0;
            }

            if (id == ID_BTN_EQ_OFF && code == BN_CLICKED) {
                int sel = (int)SendMessage(g_app->ui.hInvW, LB_GETCURSEL, 0, 0);
                if (sel >= 0 && (size_t)sel < g_app->inv.weaponsCount()) {
                    g_app->inv.equipOffhand((size_t)sel);
                    push_log(*g_app, "Equipped off-hand weapon (disables shield).");
                    full_refresh(*g_app);
                }
                return 0;
            }

            if (id == ID_BTN_EQ_GEAR && code == BN_CLICKED) {
                int sel = (int)SendMessage(g_app->ui.hInvG, LB_GETCURSEL, 0, 0);
                if (sel >= 0 && (size_t)sel < g_app->inv.gearCount()) {
                    const auto& g = g_app->inv.gearAt((size_t)sel);
                    g_app->inv.equipGear((size_t)sel);
                    std::ostringstream os; os << "Equipped " << slotName(g.slot) << ".";
                    push_log(*g_app, os.str());
                    full_refresh(*g_app);
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

            if (id == ID_CHK_AUTO && code == BN_CLICKED) {
                LRESULT state = SendMessage(g_app->ui.hChkAuto, BM_GETCHECK, 0, 0);
                g_app->autoEquipBetter = (state == BST_CHECKED);
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
        WS_OVERLAPPEDWINDOW|WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT, 1200, 760,
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
