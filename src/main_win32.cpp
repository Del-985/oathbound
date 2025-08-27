#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <string>
#include <vector>
#include <sstream>
#include <algorithm>
#include <cmath>

#include "core/rng.hpp"
#include "game/rarity.hpp"
#include "game/affix.hpp"
#include "game/slots.hpp"
#include "game/item.hpp"
#include "game/actor.hpp"
#include "game/inventory.hpp"
#include "game/loot_tables.hpp"
#include "game/combat_math.hpp"

using namespace game;

// ---------- control IDs ----------
enum {
    ID_LB_WEAP = 1001,
    ID_LB_GEAR,
    ID_LB_ENEMIES,
    ID_BTN_EQ_MAIN,
    ID_BTN_EQ_OFF,
    ID_BTN_EQ_GEAR,
    ID_BTN_BEST,
    ID_BTN_NEXT,
    ID_BTN_RESET,
    ID_EDIT_LOG,
    ID_STATIC_PLAYER,
    ID_CHK_AUTO
};

struct UI {
    HWND hWeap{}, hGear{}, hEnemies{};
    HWND hEqMain{}, hEqOff{}, hEqGear{}, hBest{};
    HWND hNext{}, hReset{}, hLog{}, hPlayer{}, hAuto{};
};

struct App {
    core::RNG rng{1337};
    LootTables loot = makeDefaultLoot();
    Inventory inv;
    Actor player{ "Player", 60, 60, 1, Item{} };
    std::vector<Actor> enemies;
    bool autoEquipBetter = true;
    std::vector<std::string> log;
    UI ui;
};

static App* g = nullptr;

// ---------- helpers ----------
static void push_log(const std::string& s) {
    if (g->log.size() > 800) g->log.erase(g->log.begin(), g->log.begin()+400);
    g->log.push_back(s);
}

static Item mkWeapon(const std::string& name,int mn,int mx,bool twoH=false){
    Item w; w.name=name; w.kind=ItemKind::Weapon; w.slot=Slot::Weapon; w.baseMin=mn; w.baseMax=mx; w.twoHanded=twoH; return w;
}
static Item mkShield(const std::string& name,int armor){
    Item g; g.name=name; g.kind=ItemKind::Gear; g.slot=Slot::Offhand; g.armorBonus=armor; return g;
}
static Item mkGear(const std::string& name, Slot slot, int armor){
    Item g; g.name=name; g.kind=ItemKind::Gear; g.slot=slot; g.armorBonus=armor; return g;
}

static std::vector<Actor> make_enemies_random(core::RNG& rng) {
    struct Arch { const char* name; int hpMin,hpMax; int armorMin,armorMax; int wMin,wMax; };
    std::vector<Arch> arch = {
        {"Goblin",   16, 24, 0, 1, 1, 4},
        {"Raider",   22, 30, 0, 1, 2, 6},
        {"Brute",    32, 44, 1, 3, 3, 7},
        {"Skirmisher",18, 26, 0, 1, 2, 5},
        {"Boneguard", 20, 34, 1, 2, 3, 6}
    };
    int n = rng.i(3,5);
    std::vector<Actor> out; out.reserve((size_t)n);
    for (int i=0;i<n;++i) {
        const auto& a = arch[(size_t)rng.i(0,(int)arch.size()-1)];
        int hp = rng.i(a.hpMin, a.hpMax);
        int ar = rng.i(a.armorMin, a.armorMax);
        Item w = mkWeapon(std::string(a.name)=="Brute"?"Club":(std::string(a.name)=="Boneguard"?"Rusty Blade":"Shiv"), a.wMin,a.wMax);
        out.push_back( Actor{ a.name, hp, hp, ar, w } );
    }
    return out;
}

static void refresh_log() {
    std::ostringstream os;
    for (auto& s: g->log) os << s << "\r\n";
    SetWindowTextA(g->ui.hLog, os.str().c_str());
    SendMessageA(g->ui.hLog, EM_SETSEL, (WPARAM)-1, (LPARAM)-1);
    SendMessageA(g->ui.hLog, EM_SCROLLCARET, 0, 0);
}

static void refresh_player() {
    GearBonuses b = g->inv.bonuses();
    const Item* mh = g->inv.equipped();
    std::ostringstream os;
    os << "Player  HP " << g->player.hp << "/" << g->player.maxHP
       << "  Armor " << (1 + b.armor) << "\r\n"
       << "Main-hand: " << (mh ? mh->label() : std::string("(none)"));
    SetWindowTextA(g->ui.hPlayer, os.str().c_str());
}

static void refresh_weapons() {
    SendMessageA(g->ui.hWeap, LB_RESETCONTENT, 0, 0);
    for (size_t i=0;i<g->inv.weaponsCount();++i) {
        const auto& w = g->inv.weaponAt(i);
        std::ostringstream os; os << w.label();
        SendMessageA(g->ui.hWeap, LB_ADDSTRING, 0, (LPARAM)os.str().c_str());
    }
}

static void refresh_gear() {
    SendMessageA(g->ui.hGear, LB_RESETCONTENT, 0, 0);
    for (size_t i=0;i<g->inv.gearCount();++i) {
        const auto& it = g->inv.gearAt(i);
        std::ostringstream os; os << "(" << slotName(it.slot) << ") " << it.label();
        SendMessageA(g->ui.hGear, LB_ADDSTRING, 0, (LPARAM)os.str().c_str());
    }
}

static void refresh_enemies() {
    SendMessageA(g->ui.hEnemies, LB_RESETCONTENT, 0, 0);
    for (size_t i=0;i<g->enemies.size();++i) {
        const auto& e = g->enemies[i];
        std::ostringstream os; os << e.name << "  HP " << std::max(0,e.hp) << "/" << e.maxHP << "  Arm " << e.armor;
        SendMessageA(g->ui.hEnemies, LB_ADDSTRING, 0, (LPARAM)os.str().c_str());
    }
}

// One player+enemy round
static void do_round() {
    if (!g->player.alive()) { push_log("You are dead. Reset first."); refresh_log(); return; }

    GearBonuses b = g->inv.bonuses();
    const Item* mh = g->inv.equipped();
    if (!mh) { push_log("No main-hand weapon equipped."); refresh_log(); return; }

    // Target: first alive
    auto it = std::find_if(g->enemies.begin(), g->enemies.end(), [](const Actor& a){return a.alive();});
    if (it == g->enemies.end()) { push_log("No enemies alive. Reset."); refresh_log(); return; }
    Actor& target = *it;

    // Main-hand hits
    int hits = std::max(1, (int)std::round(expectedAPS(*mh, b.attackSpeed)));
    for (int h=0; h<hits && target.alive(); ++h) {
        int dmg = g->player.attack(target, g->rng, b.pctDamage, b.critChance);
        target.hp -= dmg;
        std::ostringstream os; os << "You hit " << target.name << " for " << dmg
                                  << " (" << std::max(0,target.hp) << "/" << target.maxHP << ")";
        push_log(os.str());
    }

    // Off-hand (one swing)
    if (target.alive()) {
        if (const Item* oh = g->inv.equippedOffhand()) {
            int d = rollDamageWithBonuses(*oh, g->rng, b.pctDamage, b.critChance);
            d = std::max(0, d - target.armor);
            target.hp -= d;
            std::ostringstream os; os << "You (OH) hit " << target.name << " for " << d
                                      << " (" << std::max(0,target.hp) << "/" << target.maxHP << ")";
            push_log(os.str());
        }
    }

    // Death & drop
    if (!target.alive()) {
        std::ostringstream os; os << target.name << " is slain!"; push_log(os.str());

        if (g->loot.rollIsGear(g->rng)) {
            Item ge = g->loot.rollGear(g->rng, 1);
            g->inv.addGear(ge);
            push_log(std::string("Drop: ") + ge.label());
            refresh_gear();
        } else {
            Item we = g->loot.rollWeapon(g->rng, 1);
            size_t idx = g->inv.addWeapon(we);
            push_log(std::string("Drop: ") + we.label());
            if (g->autoEquipBetter && mh) {
                double cur  = expectedDPR(*mh, b.pctDamage, b.critChance, b.attackSpeed);
                double cand = expectedDPR(g->inv.weaponAt(idx), b.pctDamage, b.critChance, b.attackSpeed);
                if (cand > cur) {
                    g->inv.equip(idx);
                    g->player.weapon = g->inv.weaponAt(idx);
                    push_log("Auto-equipped better weapon.");
                }
            }
            refresh_weapons();
        }
    }

    // Enemy swings
    for (auto& e : g->enemies) {
        if (!e.alive() || !g->player.alive()) continue;
        int swings = std::max(1, (int)std::round(1.0 + e.weapon.attackSpeed()));
        for (int s=0; s<swings && g->player.alive(); ++s) {
            int d = e.attack(g->player, g->rng);
            g->player.hp -= d;
            std::ostringstream os; os << e.name << " hits you for " << d
                                      << " (You: " << std::max(0,g->player.hp) << "/" << g->player.maxHP << ")";
            push_log(os.str());
        }
    }

    if (!g->player.alive()) push_log("Defeat. You died.");
    else if (std::none_of(g->enemies.begin(), g->enemies.end(), [](const Actor& a){return a.alive();}))
        push_log("Victory! All enemies defeated.");

    refresh_enemies();
    refresh_player();
    refresh_log();
}

static void reset_battle() {
    if (auto mh = g->inv.equipped()) g->player.weapon = *mh;
    g->player.hp = g->player.maxHP;
    g->enemies = make_enemies_random(g->rng);
    push_log("Battle reset.");
    refresh_enemies();
    refresh_player();
    refresh_log();
}

// ---------- layout ----------
static void layout_controls(HWND hWnd) {
    RECT rc; GetClientRect(hWnd, &rc);
    int w = rc.right - rc.left, h = rc.bottom - rc.top;
    int pad = 8, leftW = w * 2 / 5;

    // Left column
    MoveWindow(g->ui.hPlayer, pad, pad, leftW - 2*pad, 60, TRUE);
    MoveWindow(g->ui.hEnemies, pad, pad + 60 + pad, leftW - 2*pad, h - (60 + 5*pad + 60), TRUE);

    MoveWindow(g->ui.hNext,  pad, h - pad - 26, 110, 26, TRUE);
    MoveWindow(g->ui.hReset, pad + 120, h - pad - 26, 110, 26, TRUE);

    // Right column
    int rx = leftW + pad, rw = w - rx - pad;
    int topH = (h - 3*pad - 26) / 2;

    MoveWindow(g->ui.hWeap, rx, pad, rw, topH - 30, TRUE);
    MoveWindow(g->ui.hEqMain, rx, pad + topH - 26, 120, 26, TRUE);
    MoveWindow(g->ui.hEqOff,  rx + 130, pad + topH - 26, 140, 26, TRUE);

    MoveWindow(g->ui.hGear, rx, pad + topH + pad, rw, topH - 30, TRUE);
    MoveWindow(g->ui.hEqGear, rx, pad + 2*topH, 120, 26, TRUE);
    MoveWindow(g->ui.hBest,   rx + 130, pad + 2*topH, 120, 26, TRUE);

    MoveWindow(g->ui.hAuto, rx + 260, pad + 2*topH, 160, 26, TRUE);

    MoveWindow(g->ui.hLog, rx, h - pad - 26, rw, 26, TRUE);
}

// ---------- Win32 ----------
LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CREATE: {
            g = new App();

            // starter inventory
            size_t s = g->inv.addWeapon(mkWeapon("Rusty Sword", 2, 6));
            g->inv.equip(s);
            g->player.weapon = g->inv.weaponAt(s);
            g->inv.addGear(mkShield("Wooden Shield", 2));
            g->inv.addGear(mkGear("Leather Armor", Slot::Armor, 3));

            g->enemies = make_enemies_random(g->rng);
            push_log("Welcome! Equip items and click Next Round.");

            HFONT font = (HFONT)GetStockObject(DEFAULT_GUI_FONT);

            g->ui.hPlayer  = CreateWindowExA(0, "STATIC", "", WS_CHILD|WS_VISIBLE|SS_LEFT,
                                 0,0,0,0, hWnd, (HMENU)ID_STATIC_PLAYER, GetModuleHandle(nullptr), nullptr);
            g->ui.hEnemies = CreateWindowExA(WS_EX_CLIENTEDGE, "LISTBOX", "",
                                 WS_CHILD|WS_VISIBLE|WS_VSCROLL, 0,0,0,0, hWnd, (HMENU)ID_LB_ENEMIES, GetModuleHandle(nullptr), nullptr);
            g->ui.hNext    = CreateWindowExA(0, "BUTTON", "Next Round",
                                 WS_CHILD|WS_VISIBLE, 0,0,0,0, hWnd, (HMENU)ID_BTN_NEXT, GetModuleHandle(nullptr), nullptr);
            g->ui.hReset   = CreateWindowExA(0, "BUTTON", "Reset Battle",
                                 WS_CHILD|WS_VISIBLE, 0,0,0,0, hWnd, (HMENU)ID_BTN_RESET, GetModuleHandle(nullptr), nullptr);

            g->ui.hWeap    = CreateWindowExA(WS_EX_CLIENTEDGE, "LISTBOX", "",
                                 WS_CHILD|WS_VISIBLE|WS_VSCROLL, 0,0,0,0, hWnd, (HMENU)ID_LB_WEAP, GetModuleHandle(nullptr), nullptr);
            g->ui.hEqMain  = CreateWindowExA(0, "BUTTON", "Equip Weapon",
                                 WS_CHILD|WS_VISIBLE, 0,0,0,0, hWnd, (HMENU)ID_BTN_EQ_MAIN, GetModuleHandle(nullptr), nullptr);
            g->ui.hEqOff   = CreateWindowExA(0, "BUTTON", "Equip Off-hand",
                                 WS_CHILD|WS_VISIBLE, 0,0,0,0, hWnd, (HMENU)ID_BTN_EQ_OFF, GetModuleHandle(nullptr), nullptr);

            g->ui.hGear    = CreateWindowExA(WS_EX_CLIENTEDGE, "LISTBOX", "",
                                 WS_CHILD|WS_VISIBLE|WS_VSCROLL, 0,0,0,0, hWnd, (HMENU)ID_LB_GEAR, GetModuleHandle(nullptr), nullptr);
            g->ui.hEqGear  = CreateWindowExA(0, "BUTTON", "Equip Gear",
                                 WS_CHILD|WS_VISIBLE, 0,0,0,0, hWnd, (HMENU)ID_BTN_EQ_GEAR, GetModuleHandle(nullptr), nullptr);
            g->ui.hBest    = CreateWindowExA(0, "BUTTON", "Equip Best",
                                 WS_CHILD|WS_VISIBLE, 0,0,0,0, hWnd, (HMENU)ID_BTN_BEST, GetModuleHandle(nullptr), nullptr);

            g->ui.hAuto    = CreateWindowExA(0, "BUTTON", "Auto-equip drops",
                                 WS_CHILD|WS_VISIBLE|BS_AUTOCHECKBOX, 0,0,0,0, hWnd, (HMENU)ID_CHK_AUTO, GetModuleHandle(nullptr), nullptr);
            SendMessage(g->ui.hAuto, BM_SETCHECK, BST_CHECKED, 0);

            g->ui.hLog     = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", "",
                                 WS_CHILD|WS_VISIBLE|ES_READONLY|ES_AUTOHSCROLL,
                                 0,0,0,0, hWnd, (HMENU)ID_EDIT_LOG, GetModuleHandle(nullptr), nullptr);

            for (HWND ctl : { g->ui.hPlayer, g->ui.hEnemies, g->ui.hNext, g->ui.hReset,
                              g->ui.hWeap, g->ui.hEqMain, g->ui.hEqOff, g->ui.hGear,
                              g->ui.hEqGear, g->ui.hBest, g->ui.hLog, g->ui.hAuto }) {
                SendMessage(ctl, WM_SETFONT, (WPARAM)font, TRUE);
            }

            refresh_weapons();
            refresh_gear();
            refresh_enemies();
            refresh_player();
            refresh_log();

            return 0;
        }

        case WM_SIZE:
            layout_controls(hWnd);
            return 0;

        case WM_COMMAND: {
            int id = LOWORD(wParam);
            int code = HIWORD(wParam);

            if (id == ID_BTN_NEXT && code == BN_CLICKED) { do_round(); return 0; }
            if (id == ID_BTN_RESET && code == BN_CLICKED) { reset_battle(); return 0; }

            if (id == ID_BTN_EQ_MAIN && code == BN_CLICKED) {
                int sel = (int)SendMessage(g->ui.hWeap, LB_GETCURSEL, 0, 0);
                if (sel >= 0 && (size_t)sel < g->inv.weaponsCount()) {
                    g->inv.equip((size_t)sel);
                    g->player.weapon = g->inv.weaponAt((size_t)sel);
                    push_log("Equipped main-hand weapon.");
                    refresh_player(); refresh_log();
                }
                return 0;
            }

            if (id == ID_BTN_EQ_OFF && code == BN_CLICKED) {
                int sel = (int)SendMessage(g->ui.hWeap, LB_GETCURSEL, 0, 0);
                if (sel >= 0 && (size_t)sel < g->inv.weaponsCount()) {
                    if (g->inv.equipOffhand((size_t)sel)) {
                        push_log("Equipped off-hand weapon (disables shield).");
                    } else {
                        push_log("Cannot equip in off-hand (two-handed or invalid).");
                    }
                    refresh_log();
                }
                return 0;
            }

            if (id == ID_BTN_EQ_GEAR && code == BN_CLICKED) {
                int sel = (int)SendMessage(g->ui.hGear, LB_GETCURSEL, 0, 0);
                if (sel >= 0 && (size_t)sel < g->inv.gearCount()) {
                    g->inv.equipGear((size_t)sel);
                    push_log("Equipped gear.");
                    refresh_player(); refresh_log();
                }
                return 0;
            }

            if (id == ID_BTN_BEST && code == BN_CLICKED) {
                if (g->inv.equipBest()) {
                    if (auto mh = g->inv.equipped()) g->player.weapon = *mh;
                    push_log("Equipped best-by-DPR.");
                    refresh_player(); refresh_log();
                }
                return 0;
            }

            if (id == ID_CHK_AUTO && code == BN_CLICKED) {
                LRESULT st = SendMessage(g->ui.hAuto, BM_GETCHECK, 0, 0);
                g->autoEquipBetter = (st == BST_CHECKED);
                return 0;
            }

            break;
        }

        case WM_DESTROY:
            if (g) { delete g; g = nullptr; }
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE, LPSTR, int nCmdShow) {
    const char* cls = "OathboundWnd";
    WNDCLASSA wc{}; wc.style=CS_HREDRAW|CS_VREDRAW; wc.lpfnWndProc=WndProc; wc.hInstance=hInst;
    wc.hCursor=LoadCursor(nullptr, IDC_ARROW); wc.hbrBackground=(HBRUSH)(COLOR_WINDOW+1); wc.lpszClassName=cls;
    RegisterClassA(&wc);

    HWND w = CreateWindowExA(0, cls, "Oathbound (Win32, Item-only)", WS_OVERLAPPEDWINDOW|WS_VISIBLE,
                             CW_USEDEFAULT, CW_USEDEFAULT, 1200, 760, nullptr, nullptr, hInst, nullptr);

    ShowWindow(w, nCmdShow); UpdateWindow(w);

    MSG msg;
    while (GetMessage(&msg,nullptr,0,0)) { TranslateMessage(&msg); DispatchMessage(&msg); }
    return (int)msg.wParam;
}