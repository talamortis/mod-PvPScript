#include "Configuration/Config.h"
#include "Player.h"
#include "Creature.h"
#include "AccountMgr.h"
#include "ScriptMgr.h"
#include "Define.h"
#include "GossipDef.h"
#include "Pet.h"
#include "LootMgr.h"
#include "Chat.h"

uint32 SummonChest, KillAnnounce;
bool spawnchestIP;
uint32 chestDespawn;
std::vector<uint32> AreatoIgnore = { 1741 /*Gurubashi*/, 2177 };

class PvPScript : public PlayerScript
{
public:
    PvPScript() : PlayerScript("PvPScript") {}

    void OnPlayerKilledByCreature(Creature* killer, Player* killed/*, bool& durabilityLoss*/)
    {
        if (!sConfigMgr->GetOption<bool>("PvPChest", true))
            return;

        if (!killer->IsPet())
            return;

        std::string name = killer->GetOwner()->GetName();

        //if killer has same IP as death player do not drop loot as its cheating!
        if (spawnchestIP)
            if (Pet* pet = killer->ToPet())
                if (Player* owner = pet->GetOwner())
                {
                    if (!CheckConditions(owner, killed))
                        return;
                }

        if (!CheckConditions(nullptr, killed))
            return;

        // if target is killed and killer is pet
        if (!killed->IsAlive() && killer->IsPet())
        {
            AnnounceKill(KillAnnounce, killed, name.c_str());
            SpawnChest(killer, killed);
        }
    }

    void OnPVPKill(Player* killer, Player* killed)
    {
        if (!sConfigMgr->GetOption<bool>("PvPChest", true))
            return;

        std::string name = killer->GetName();

        if (!CheckConditions(killer, killed))
            return;

        if (!killed->IsAlive())
        {
            SpawnChest(killer, killed);
            AnnounceKill(KillAnnounce, killed, name.c_str());
        }
    }

    void SpawnChest(Unit* killer, Player* killed)
    {
        if (GameObject* go = killer->SummonGameObject(SummonChest, killed->GetPositionX(), killed->GetPositionY(), killed->GetPositionZ(), killed->GetOrientation(), 0.0f, 0.0f, 0.0f, 0.0f, chestDespawn, false))
        {
            killer->AddGameObject(go);
            go->SetOwnerGUID(ObjectGuid::Empty); //This is so killed players can also loot the chest
            AddChestItems(killed, go);
        }
    }

    bool CheckConditions(Player* killer, Player* killed)
    {
        //if killer has same IP as death player do not drop loot as its cheating!
        if (spawnchestIP)
            if (killer->GetSession()->GetRemoteAddress() == killed->GetSession()->GetRemoteAddress())
                return false;

        // if player has Ress sickness do not spawn chest
        if (killed->HasAura(15007))
            return false;

        // If player kills self do not drop loot
        if (killer->GetGUID() == killed->GetGUID())
            return false;

        // if killer not worth honnor do not drop loot
        if (!killer->isHonorOrXPTarget(killed))
            return false;

        // Dont drop chess if player is in battleground
        if (killed->GetMap()->IsBattlegroundOrArena())
            return false;

        //Gurubashi Arena
        if (killed->GetMapId() == 0 && killed->GetZoneId() == 33)
            for (int i = 0; i < int(AreatoIgnore.size()); ++i)
                if (killed->GetAreaId() == AreatoIgnore[i])
                    return false;

        return true;
    }

    void AnnounceKill(uint32 phase, Player* killed, std::string name)
    {
        switch (phase)
        {
        case 1: //Announce in chat handler
            ChatHandler(killed->GetSession()).PSendSysMessage("You have been killed by player [%s] ", name.c_str());
            break;
        case 2: //Announce in notifaction
            killed->GetSession()->SendNotification("You have been slain by [%s]", name.c_str());
            break;
        case 3: // Announe in Notifaction and chathandler
            killed->GetSession()->SendNotification("You have been slain by [%s]", name.c_str());
            ChatHandler(killed->GetSession()).PSendSysMessage("You have been killed by player [%s] ", name.c_str());
            break;
        }
    }

    void AddChestItems(Player* killed, GameObject* go)
    {
        for (uint8 i = EQUIPMENT_SLOT_START; i < EQUIPMENT_SLOT_END; ++i)
            if (Item* pItem = killed->GetItemByPos(INVENTORY_SLOT_BAG_0, i))
            {
                uint8 slot = pItem->GetSlot();
                LootStoreItem storeItem = LootStoreItem(pItem->GetEntry(), 0, 100, 0, LOOT_MODE_DEFAULT, 0, 1, 1);
                go->loot.AddItem(storeItem);
                killed->DestroyItem(INVENTORY_SLOT_BAG_0, slot, true);
            }
    }
};

class PvPScript_conf : public WorldScript
{
public:
    PvPScript_conf() : WorldScript("PvPScriptConf") { }

    void OnBeforeConfigLoad(bool reload) override
    {
        if (!reload)
        {
            SummonChest = sConfigMgr->GetOption<uint32>("ChestID", 179697);
            KillAnnounce = sConfigMgr->GetOption<uint8>("KillAnnounce", 1);
            chestDespawn = sConfigMgr->GetOption<uint8>("ChestTimer", 120);
            spawnchestIP = sConfigMgr->GetOption<bool>("spawnchestIP", true);
        }
    }
};

void AddPvPScripts()
{
    new PvPScript();
    new PvPScript_conf();
}
