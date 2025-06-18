#ifndef PLAYER_HPP
#define PLAYER_HPP

#include <string> // For std::strncpy, std::strlen in some compilers, though cstring is more standard
#include <cstring> // For std::strncpy, std::strlen

// Include windows.h for BOOL and DWORD types
#include <windows.h>
// Ensure string operations are available
#include <cstring>


class Player {
private:
    // float members
    float x, y, z, rot_angle, gunx, guny, gunz, gunangle, dist, speed;

    // int members
    int monsterid, model_id, skin_tex_id, current_weapon, current_weapon_tex, current_car, current_frame, current_sequence, ping, frags, health, gunid, guntex, animationdir, volume, sattack, sdie, syell, sweapon, mdmg, hd, ac, hp, damage1, damage2, thaco, xp, gold, keys, rings, ability, closest, firespeed, attackspeed, applydamageonce, takedamageonce;

    // BOOL members
    BOOL draw_external_wep, bIsFiring, bIsRunning, bIsPlayerAlive, bIsPlayerInWalkMode, bStopAnimating, bIsPlayerValid;

    // char array members
    char name[256];
    char chatstr[180];
    char rname[80];
    char gunname[80];
    char texturename[80];
    char monsterweapon[80];

    // DWORD member
    DWORD RRnetID;

public:
    // Default constructor
    Player() :
        x(0.0f), y(0.0f), z(0.0f), rot_angle(0.0f), gunx(0.0f), guny(0.0f), gunz(0.0f), gunangle(0.0f), dist(0.0f), speed(0.0f),
        monsterid(0), model_id(0), skin_tex_id(0), current_weapon(0), current_weapon_tex(0), current_car(0), current_frame(0), current_sequence(0), ping(0), frags(0), health(0), gunid(0), guntex(0), animationdir(0), volume(0), sattack(0), sdie(0), syell(0), sweapon(0), mdmg(0), hd(0), ac(0), hp(0), damage1(0), damage2(0), thaco(0), xp(0), gold(0), keys(0), rings(0), ability(0), closest(0), firespeed(0), attackspeed(0), applydamageonce(0), takedamageonce(0),
        draw_external_wep(FALSE), bIsFiring(FALSE), bIsRunning(FALSE), bIsPlayerAlive(FALSE), bIsPlayerInWalkMode(FALSE), bStopAnimating(FALSE), bIsPlayerValid(FALSE),
        RRnetID(0) {
        name[0] = '\0';
        chatstr[0] = '\0';
        rname[0] = '\0';
        gunname[0] = '\0';
        texturename[0] = '\0';
        monsterweapon[0] = '\0';
    }

    // Getters and Setters for float members
    float getX() const { return x; }
    void setX(float val) { x = val; }
    float getY() const { return y; }
    void setY(float val) { y = val; }
    float getZ() const { return z; }
    void setZ(float val) { z = val; }
    float getRotAngle() const { return rot_angle; }
    void setRotAngle(float val) { rot_angle = val; }
    float getGunX() const { return gunx; }
    void setGunX(float val) { gunx = val; }
    float getGunY() const { return guny; }
    void setGunY(float val) { guny = val; }
    float getGunZ() const { return gunz; }
    void setGunZ(float val) { gunz = val; }
    float getGunAngle() const { return gunangle; }
    void setGunAngle(float val) { gunangle = val; }
    float getDist() const { return dist; }
    void setDist(float val) { dist = val; }
    float getSpeed() const { return speed; }
    void setSpeed(float val) { speed = val; }

    // Getters and Setters for int members
    int getMonsterId() const { return monsterid; }
    void setMonsterId(int val) { monsterid = val; }
    int getModelId() const { return model_id; }
    void setModelId(int val) { model_id = val; }
    int getSkinTexId() const { return skin_tex_id; }
    void setSkinTexId(int val) { skin_tex_id = val; }
    int getCurrentWeapon() const { return current_weapon; }
    void setCurrentWeapon(int val) { current_weapon = val; }
    int getCurrentWeaponTex() const { return current_weapon_tex; }
    void setCurrentWeaponTex(int val) { current_weapon_tex = val; }
    int getCurrentCar() const { return current_car; }
    void setCurrentCar(int val) { current_car = val; }
    int getCurrentFrame() const { return current_frame; }
    void setCurrentFrame(int val) { current_frame = val; }
    int getCurrentSequence() const { return current_sequence; }
    void setCurrentSequence(int val) { current_sequence = val; }
    int getPing() const { return ping; }
    void setPing(int val) { ping = val; }
    int getFrags() const { return frags; }
    void setFrags(int val) { frags = val; }
    int getHealth() const { return health; }
    void setHealth(int val) { health = val; }
    int getGunId() const { return gunid; }
    void setGunId(int val) { gunid = val; }
    int getGunTex() const { return guntex; }
    void setGunTex(int val) { guntex = val; }
    int getAnimationDir() const { return animationdir; }
    void setAnimationDir(int val) { animationdir = val; }
    int getVolume() const { return volume; }
    void setVolume(int val) { volume = val; }
    int getSAttack() const { return sattack; }
    void setSAttack(int val) { sattack = val; }
    int getSDie() const { return sdie; }
    void setSDie(int val) { sdie = val; }
    int getSYell() const { return syell; }
    void setSYell(int val) { syell = val; }
    int getSWeapon() const { return sweapon; }
    void setSWeapon(int val) { sweapon = val; }
    int getMDmg() const { return mdmg; }
    void setMDmg(int val) { mdmg = val; }
    int getHd() const { return hd; }
    void setHd(int val) { hd = val; }
    int getAc() const { return ac; }
    void setAc(int val) { ac = val; }
    int getHp() const { return hp; }
    void setHp(int val) { hp = val; }
    int getDamage1() const { return damage1; }
    void setDamage1(int val) { damage1 = val; }
    int getDamage2() const { return damage2; }
    void setDamage2(int val) { damage2 = val; }
    int getThaco() const { return thaco; }
    void setThaco(int val) { thaco = val; }
    int getXp() const { return xp; }
    void setXp(int val) { xp = val; }
    int getGold() const { return gold; }
    void setGold(int val) { gold = val; }
    int getKeys() const { return keys; }
    void setKeys(int val) { keys = val; }
    int getRings() const { return rings; }
    void setRings(int val) { rings = val; }
    int getAbility() const { return ability; }
    void setAbility(int val) { ability = val; }
    int getClosest() const { return closest; }
    void setClosest(int val) { closest = val; }
    int getFireSpeed() const { return firespeed; }
    void setFireSpeed(int val) { firespeed = val; }
    int getAttackSpeed() const { return attackspeed; }
    void setAttackSpeed(int val) { attackspeed = val; }
    int getApplyDamageOnce() const { return applydamageonce; }
    void setApplyDamageOnce(int val) { applydamageonce = val; }
    int getTakeDamageOnce() const { return takedamageonce; }
    void setTakeDamageOnce(int val) { takedamageonce = val; }

    // Getters and Setters for BOOL members
    BOOL getDrawExternalWep() const { return draw_external_wep; }
    void setDrawExternalWep(BOOL val) { draw_external_wep = val; }
    BOOL getIsFiring() const { return bIsFiring; }
    void setIsFiring(BOOL val) { bIsFiring = val; }
    BOOL getIsRunning() const { return bIsRunning; }
    void setIsRunning(BOOL val) { bIsRunning = val; }
    BOOL getIsPlayerAlive() const { return bIsPlayerAlive; }
    void setIsPlayerAlive(BOOL val) { bIsPlayerAlive = val; }
    BOOL getIsPlayerInWalkMode() const { return bIsPlayerInWalkMode; }
    void setIsPlayerInWalkMode(BOOL val) { bIsPlayerInWalkMode = val; }
    BOOL getStopAnimating() const { return bStopAnimating; }
    void setStopAnimating(BOOL val) { bStopAnimating = val; }
    BOOL getIsPlayerValid() const { return bIsPlayerValid; }
    void setIsPlayerValid(BOOL val) { bIsPlayerValid = val; }

    // Getters and Setters for char array members
    const char* getName() const { return name; }
    void setName(const char* val) {
        if (val) {
            std::strncpy(name, val, sizeof(name) - 1);
            name[sizeof(name) - 1] = '\0';
        } else {
            name[0] = '\0';
        }
    }

    const char* getChatStr() const { return chatstr; }
    void setChatStr(const char* val) {
        if (val) {
            std::strncpy(chatstr, val, sizeof(chatstr) - 1);
            chatstr[sizeof(chatstr) - 1] = '\0';
        } else {
            chatstr[0] = '\0';
        }
    }

    const char* getRName() const { return rname; }
    void setRName(const char* val) {
        if (val) {
            std::strncpy(rname, val, sizeof(rname) - 1);
            rname[sizeof(rname) - 1] = '\0';
        } else {
            rname[0] = '\0';
        }
    }

    const char* getGunName() const { return gunname; }
    void setGunName(const char* val) {
        if (val) {
            std::strncpy(gunname, val, sizeof(gunname) - 1);
            gunname[sizeof(gunname) - 1] = '\0';
        } else {
            gunname[0] = '\0';
        }
    }

    const char* getTextureName() const { return texturename; }
    void setTextureName(const char* val) {
        if (val) {
            std::strncpy(texturename, val, sizeof(texturename) - 1);
            texturename[sizeof(texturename) - 1] = '\0';
        } else {
            texturename[0] = '\0';
        }
    }

    const char* getMonsterWeapon() const { return monsterweapon; }
    void setMonsterWeapon(const char* val) {
        if (val) {
            std::strncpy(monsterweapon, val, sizeof(monsterweapon) - 1);
            monsterweapon[sizeof(monsterweapon) - 1] = '\0';
        } else {
            monsterweapon[0] = '\0';
        }
    }

    // Getter and Setter for DWORD member
    DWORD getRRnetID() const { return RRnetID; }
    void setRRnetID(DWORD val) { RRnetID = val; }
};

#endif // PLAYER_HPP
