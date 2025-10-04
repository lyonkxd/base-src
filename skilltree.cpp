/**
 * The Forgotten Server - a free and open-source MMORPG server emulator
 * Copyright (C) 2019  Mark Samman <mark.samman@gmail.com>
 *
 * SKILL TREE SYSTEM - Implementação
 */

#include "otpch.h"
#include "skilltree.h"
#include "player.h"
#include "game.h"
#include "tools.h"

extern Game g_game;

// =====================================================
// IMPLEMENTAÇÃO: SkillTreeBonus
// =====================================================

std::string SkillTreeBonus::bonusTypeToString(SkillTreeBonusType type) {
    switch(type) {
        // Atributos base
        case BONUS_STRENGTH: return "strength";
        case BONUS_DEXTERITY: return "dexterity";
        case BONUS_INTELLIGENCE: return "intelligence";
        case BONUS_VITALITY: return "vitality";
        case BONUS_WISDOM: return "wisdom";
        case BONUS_SPIRIT: return "spirit";
        case BONUS_SPEED: return "speed";
        
        // Absorções
        case BONUS_ABSORB_FIRE: return "absorb_fire";
        case BONUS_ABSORB_ARCANE: return "absorb_arcane";
        case BONUS_ABSORB_WATER: return "absorb_water";
        case BONUS_ABSORB_ICE: return "absorb_ice";
        case BONUS_ABSORB_HOLY: return "absorb_holy";
        case BONUS_ABSORB_DEATH: return "absorb_death";
        case BONUS_ABSORB_DROWN: return "absorb_drown";
        case BONUS_ABSORB_LIFEDRAIN: return "absorb_lifedrain";
        case BONUS_ABSORB_MANADRAIN: return "absorb_manadrain";
        case BONUS_ABSORB_EARTH: return "absorb_earth";
        case BONUS_ABSORB_PHYSICAL: return "absorb_physical";
        
        // Dano incremental
        case BONUS_DAMAGE_FIRE: return "damage_fire";
        case BONUS_DAMAGE_ARCANE: return "damage_arcane";
        case BONUS_DAMAGE_WATER: return "damage_water";
        case BONUS_DAMAGE_ICE: return "damage_ice";
        case BONUS_DAMAGE_HOLY: return "damage_holy";
        case BONUS_DAMAGE_DEATH: return "damage_death";
        case BONUS_DAMAGE_DROWN: return "damage_drown";
        case BONUS_DAMAGE_LIFEDRAIN: return "damage_lifedrain";
        case BONUS_DAMAGE_MANADRAIN: return "damage_manadrain";
        case BONUS_DAMAGE_EARTH: return "damage_earth";
        case BONUS_DAMAGE_PHYSICAL: return "damage_physical";
        
        // Bônus especiais
        case BONUS_RARITY_LOOT: return "rarity_loot";
        case BONUS_EXP_GLOBAL: return "exp_global";
        case BONUS_LOOT_GLOBAL: return "loot_global";
        
        // Combat
        case BONUS_CRITICAL_CHANCE: return "critical_chance";
        case BONUS_CRITICAL_AMOUNT: return "critical_amount";
        case BONUS_LIFE_LEECH_CHANCE: return "life_leech_chance";
        case BONUS_LIFE_LEECH_AMOUNT: return "life_leech_amount";
        case BONUS_MANA_LEECH_CHANCE: return "mana_leech_chance";
        case BONUS_MANA_LEECH_AMOUNT: return "mana_leech_amount";
        
        // Skills
        case BONUS_SKILL_SWORD: return "skill_sword";
        case BONUS_SKILL_CLUB: return "skill_club";
        case BONUS_SKILL_AXE: return "skill_axe";
        case BONUS_SKILL_DISTANCE: return "skill_distance";
        case BONUS_SKILL_SHIELDING: return "skill_shielding";
        case BONUS_SKILL_MAGIC: return "skill_magic";
        
        // Profissões
        case BONUS_SKILL_CRAFTING: return "skill_crafting";
        case BONUS_SKILL_HERBALIST: return "skill_herbalist";
        case BONUS_SKILL_JEWELSMITH: return "skill_jewelsmith";
        case BONUS_SKILL_ARMORSMITH: return "skill_armorsmith";
        case BONUS_SKILL_WEAPONSMITH: return "skill_weaponsmith";
        case BONUS_SKILL_WOODCUTTING: return "skill_woodcutting";
        case BONUS_SKILL_MINING: return "skill_mining";
        
        default: return "unknown";
    }
}

// =====================================================
// IMPLEMENTAÇÃO: SkillTreeNode
// =====================================================

std::vector<SkillTreeBonus> SkillTreeNode::getBonusesForVocation(uint16_t vocationId) const {
    std::vector<SkillTreeBonus> result;
    
    for (const auto& bonus : bonuses) {
        // Se o bônus não tem vocação específica (0) ou é para esta vocação
        if (bonus.getVocationId() == 0 || bonus.getVocationId() == vocationId) {
            result.push_back(bonus);
        }
    }
    
    return result;
}

// =====================================================
// IMPLEMENTAÇÃO: PlayerSkillTree
// =====================================================

bool PlayerSkillTree::loadFromDatabase() {
    if (!player) {
        return false;
    }
    
    Database& db = Database::getInstance();
    
    // Carrega os pontos do player
    std::ostringstream query;
    query << "SELECT `available_points`, `total_points_earned`, `points_spent` "
          << "FROM `player_skilltree_points` WHERE `player_id` = " << player->getGUID();
    
    DBResult_ptr result = db.storeQuery(query.str());
    if (result) {
        availablePoints = result->getNumber<uint32_t>("available_points");
        totalPointsEarned = result->getNumber<uint32_t>("total_points_earned");
        pointsSpent = result->getNumber<uint32_t>("points_spent");
    } else {
        // Primeira vez do player, calcular baseado no level
        updatePointsBasedOnLevel();
        saveToDatabase();
    }
    
    // Carrega os nós alocados
    allocatedNodes.clear();
    query.str(std::string());
    query << "SELECT `node_id` FROM `player_skilltree` WHERE `player_id` = " << player->getGUID();
    
    result = db.storeQuery(query.str());
    if (result) {
        do {
            uint32_t nodeId = result->getNumber<uint32_t>("node_id");
            allocatedNodes.insert(nodeId);
        } while (result->next());
    }
    
    // Recalcula e aplica os bônus
    calculateBonuses();
    applyBonusesToPlayer();
    
    return true;
}

bool PlayerSkillTree::saveToDatabase() {
    if (!player) {
        return false;
    }
    
    Database& db = Database::getInstance();
    std::ostringstream query;
    
    // Salva/atualiza os pontos
    query << "INSERT INTO `player_skilltree_points` (`player_id`, `available_points`, `total_points_earned`, `points_spent`) "
          << "VALUES (" << player->getGUID() << ", " << availablePoints << ", " 
          << totalPointsEarned << ", " << pointsSpent << ") "
          << "ON DUPLICATE KEY UPDATE "
          << "`available_points` = " << availablePoints << ", "
          << "`total_points_earned` = " << totalPointsEarned << ", "
          << "`points_spent` = " << pointsSpent;
    
    if (!db.executeQuery(query.str())) {
        return false;
    }
    
    return true;
}

void PlayerSkillTree::updatePointsBasedOnLevel() {
    if (!player) {
        return;
    }
    
    uint32_t playerLevel = player->getLevel();
    uint32_t newTotalPoints = 0;
    
    // Calcula quantos pontos deveria ter (1 ponto por level de 100 a 250)
    if (playerLevel >= SKILLTREE_MIN_LEVEL) {
        if (playerLevel > SKILLTREE_MAX_LEVEL) {
            newTotalPoints = SKILLTREE_MAX_POINTS;
        } else {
            newTotalPoints = playerLevel - SKILLTREE_MIN_LEVEL + 1;
        }
    }
    
    // Se ganhou novos pontos
    if (newTotalPoints > totalPointsEarned) {
        uint32_t pointsGained = newTotalPoints - totalPointsEarned;
        availablePoints += pointsGained;
        totalPointsEarned = newTotalPoints;
        
        player->sendTextMessage(MESSAGE_EVENT_ADVANCE, 
            "Você ganhou " + std::to_string(pointsGained) + " ponto(s) de Skill Tree!");
        
        saveToDatabase();
    }
}

bool PlayerSkillTree::allocateNode(uint32_t nodeId) {
    std::string errorMsg;
    
    // Verifica se pode alocar
    if (!canAllocateNode(nodeId, errorMsg)) {
        if (player) {
            player->sendTextMessage(MESSAGE_STATUS_SMALL, errorMsg);
        }
        return false;
    }
    
    // Aloca o nó
    allocatedNodes.insert(nodeId);
    availablePoints--;
    pointsSpent++;
    
    // Salva no banco
    Database& db = Database::getInstance();
    std::ostringstream query;
    query << "INSERT INTO `player_skilltree` (`player_id`, `node_id`) "
          << "VALUES (" << player->getGUID() << ", " << nodeId << ")";
    
    if (!db.executeQuery(query.str())) {
        // Rollback em caso de erro
        allocatedNodes.erase(nodeId);
        availablePoints++;
        pointsSpent--;
        return false;
    }
    
    saveToDatabase();
    
    // Recalcula e aplica os bônus
    calculateBonuses();
    applyBonusesToPlayer();
    
    // Envia atualização para o client
    sendToClient();
    
    if (player) {
        SkillTreeNode* node = SkillTreeManager::getInstance().getNode(nodeId);
        if (node) {
            player->sendTextMessage(MESSAGE_EVENT_ADVANCE, 
                "Você alocou: " + node->getNodeName());
        }
    }
    
    return true;
}

bool PlayerSkillTree::deallocateNode(uint32_t nodeId) {
    // Verifica se o nó está alocado
    if (allocatedNodes.find(nodeId) == allocatedNodes.end()) {
        return false;
    }
    
    // TODO: Verificar se algum nó filho depende deste (não pode desalocar se sim)
    
    // Desaloca
    allocatedNodes.erase(nodeId);
    availablePoints++;
    pointsSpent--;
    
    // Remove do banco
    Database& db = Database::getInstance();
    std::ostringstream query;
    query << "DELETE FROM `player_skilltree` WHERE `player_id` = " 
          << player->getGUID() << " AND `node_id` = " << nodeId;
    
    db.executeQuery(query.str());
    saveToDatabase();
    
    // Recalcula e aplica os bônus
    calculateBonuses();
    applyBonusesToPlayer();
    
    // Envia atualização para o client
    sendToClient();
    
    return true;
}

uint64_t PlayerSkillTree::resetTree() {
    if (!player) {
        return 0;
    }
    
    // Calcula o custo (250 gold por level)
    uint64_t cost = player->getLevel() * SKILLTREE_RESET_COST_PER_LEVEL;
    
    // Verifica se tem gold suficiente
    if (!g_game.removeMoney(player, cost, 0)) {
        player->sendTextMessage(MESSAGE_STATUS_SMALL, 
            "Você precisa de " + std::to_string(cost) + " gold coins para resetar a Skill Tree.");
        return 0;
    }
    
    // Remove todos os nós alocados
    Database& db = Database::getInstance();
    std::ostringstream query;
    query << "DELETE FROM `player_skilltree` WHERE `player_id` = " << player->getGUID();
    db.executeQuery(query.str());
    
    // Retorna os pontos
    availablePoints = totalPointsEarned;
    pointsSpent = 0;
    allocatedNodes.clear();
    
    saveToDatabase();
    
    // Limpa os bônus
    cachedBonuses.clear();
    applyBonusesToPlayer();
    
    // Envia atualização para o client
    sendToClient();
    
    player->sendTextMessage(MESSAGE_EVENT_ADVANCE, 
        "Sua Skill Tree foi resetada! Você recuperou " + 
        std::to_string(availablePoints) + " pontos.");
    
    return cost;
}

bool PlayerSkillTree::canAllocateNode(uint32_t nodeId, std::string& errorMessage) const {
    // Verifica se o nó existe
    SkillTreeNode* node = SkillTreeManager::getInstance().getNode(nodeId);
    if (!node) {
        errorMessage = "Nó não encontrado.";
        return false;
    }
    
    // Verifica se tem pontos disponíveis
    if (availablePoints == 0) {
        errorMessage = "Você não tem pontos disponíveis.";
        return false;
    }
    
    // Verifica se já foi alocado
    if (hasNodeAllocated(nodeId)) {
        errorMessage = "Este nó já foi alocado.";
        return false;
    }
    
    // Verifica o nível requerido
    if (player && player->getLevel() < node->getRequiredLevel()) {
        errorMessage = "Você precisa do nível " + 
            std::to_string(node->getRequiredLevel()) + " para alocar este nó.";
        return false;
    }
    
    // Verifica se o nó está conectado (reachable)
    if (!isNodeReachable(nodeId)) {
        errorMessage = "Este nó não está conectado à sua árvore.";
        return false;
    }
    
    return true;
}

bool PlayerSkillTree::isNodeReachable(uint32_t nodeId) const {
    SkillTreeNode* node = SkillTreeManager::getInstance().getNode(nodeId);
    if (!node) {
        return false;
    }
    
    // Se é um nó inicial, sempre está disponível
    if (node->isStarting()) {
        return true;
    }
    
    // Se não tem nós alocados ainda, só pode alocar nós iniciais
    if (allocatedNodes.empty()) {
        return false;
    }
    
    // Verifica se algum dos nós conectados a este já foi alocado
    const auto& connections = node->getConnections();
    for (uint32_t connectedNodeId : connections) {
        if (hasNodeAllocated(connectedNodeId)) {
            return true; // Está conectado a um nó já alocado
        }
    }
    
    // Verifica o caminho inverso (se algum nó alocado está conectado a este)
    for (uint32_t allocatedNodeId : allocatedNodes) {
        SkillTreeNode* allocatedNode = SkillTreeManager::getInstance().getNode(allocatedNodeId);
        if (allocatedNode && allocatedNode->isConnectedTo(nodeId)) {
            return true;
        }
    }
    
    return false;
}

void PlayerSkillTree::calculateBonuses() {
    if (!player) {
        return;
    }
    
    // Limpa o cache
    cachedBonuses.clear();
    
    // Percorre todos os nós alocados
    for (uint32_t nodeId : allocatedNodes) {
        SkillTreeNode* node = SkillTreeManager::getInstance().getNode(nodeId);
        if (!node) {
            continue;
        }
        
        // Pega os bônus válidos para a vocação do player
        std::vector<SkillTreeBonus> nodeBonuses = node->getBonusesForVocation(player->getVocationId());
        
        // Acumula os bônus
        for (const auto& bonus : nodeBonuses) {
            cachedBonuses[bonus.getBonusType()] += bonus.getBonusValue();
        }
    }
}

void PlayerSkillTree::applyBonusesToPlayer() {
    if (!player) {
        return;
    }
    
    // Aqui você aplicará os bônus ao player
    // Isso depende de como você quer implementar os bônus no seu servidor
    // Exemplos:
    
    // Aplicar atributos base (você precisará adicionar esses campos no Player)
    // player->setSkillTreeStrength(cachedBonuses[BONUS_STRENGTH]);
    // player->setSkillTreeDexterity(cachedBonuses[BONUS_DEXTERITY]);
    // etc...
    
    // Ou usar storages para armazenar os valores
    // player->addStorageValue(STORAGE_SKILLTREE_STRENGTH, cachedBonuses[BONUS_STRENGTH]);
    
    // Recalcular stats do player
    // player->sendStats();
    
    std::cout << "[SkillTree] Bônus aplicados ao player " << player->getName() << std::endl;
}

void PlayerSkillTree::sendToClient() {
    if (!player) {
        return;
    }
    
    // Aqui você enviará um packet customizado para o OTClient
    // Vamos implementar isso depois no protocolo
    // Por enquanto, apenas log
    
    std::cout << "[SkillTree] Enviando skill tree para " << player->getName() 
              << " - Pontos: " << availablePoints 
              << " - Nós alocados: " << allocatedNodes.size() << std::endl;
}

// =====================================================
// IMPLEMENTAÇÃO: SkillTreeManager
// =====================================================

bool SkillTreeManager::loadFromDatabase() {
    clear();
    
    std::cout << "[SkillTreeManager] Carregando skill tree do banco de dados..." << std::endl;
    
    if (!loadNodes()) {
        std::cout << "[SkillTreeManager] ERRO ao carregar nós!" << std::endl;
        return false;
    }
    
    if (!loadConnections()) {
        std::cout << "[SkillTreeManager] ERRO ao carregar conexões!" << std::endl;
        return false;
    }
    
    if (!loadBonuses()) {
        std::cout << "[SkillTreeManager] ERRO ao carregar bônus!" << std::endl;
        return false;
    }
    
    std::cout << "[SkillTreeManager] Skill tree carregada com sucesso!" << std::endl;
    std::cout << "[SkillTreeManager] Total de nós: " << nodes.size() << std::endl;
    
    return true;
}

void SkillTreeManager::clear() {
    nodes.clear();
}

SkillTreeNode* SkillTreeManager::getNode(uint32_t nodeId) {
    auto it = nodes.find(nodeId);
    if (it != nodes.end()) {
        return &it->second;
    }
    return nullptr;
}

std::vector<SkillTreeNode*> SkillTreeManager::getNodesByType(SkillTreeNodeType type) {
    std::vector<SkillTreeNode*> result;
    
    for (auto& pair : nodes) {
        if (pair.second.getNodeType() == type) {
            result.push_back(&pair.second);
        }
    }
    
    return result;
}

std::vector<SkillTreeNode*> SkillTreeManager::getStartingNodes() {
    std::vector<SkillTreeNode*> result;
    
    for (auto& pair : nodes) {
        if (pair.second.isStarting()) {
            result.push_back(&pair.second);
        }
    }
    
    return result;
}

void SkillTreeManager::reloadFromDatabase() {
    clear();
    loadFromDatabase();
}

std::string SkillTreeManager::getStatistics() const {
    std::ostringstream ss;
    
    uint32_t smallNodes = 0, mediumNodes = 0, largeNodes = 0, startingNodes = 0;
    uint32_t totalConnections = 0;
    
    for (const auto& pair : nodes) {
        const SkillTreeNode& node = pair.second;
        
        switch(node.getNodeType()) {
            case SKILLTREE_NODE_SMALL: smallNodes++; break;
            case SKILLTREE_NODE_MEDIUM: mediumNodes++; break;
            case SKILLTREE_NODE_LARGE: largeNodes++; break;
        }
        
        if (node.isStarting()) {
            startingNodes++;
        }
        
        totalConnections += node.getConnections().size();
    }
    
    ss << "=== Skill Tree Statistics ===" << std::endl;
    ss << "Total de nós: " << nodes.size() << std::endl;
    ss << "Nós pequenos: " << smallNodes << std::endl;
    ss << "Nós médios: " << mediumNodes << std::endl;
    ss << "Nós grandes (keystones): " << largeNodes << std::endl;
    ss << "Nós iniciais: " << startingNodes << std::endl;
    ss << "Total de conexões: " << totalConnections << std::endl;
    
    return ss.str();
}

bool SkillTreeManager::loadNodes() {
    Database& db = Database::getInstance();
    
    std::ostringstream query;
    query << "SELECT `id`, `node_name`, `node_type`, `position_x`, `position_y`, "
          << "`sprite_id`, `description`, `required_level`, `is_starting_node` "
          << "FROM `skilltree_nodes`";
    
    DBResult_ptr result = db.storeQuery(query.str());
    if (!result) {
        return true; // Sem nós no banco, mas não é erro
    }
    
    do {
        uint32_t id = result->getNumber<uint32_t>("id");
        std::string name = result->getString("node_name");
        SkillTreeNodeType type = static_cast<SkillTreeNodeType>(result->getNumber<uint8_t>("node_type"));
        float posX = result->getNumber<float>("position_x");
        float posY = result->getNumber<float>("position_y");
        uint32_t spriteId = result->getNumber<uint32_t>("sprite_id");
        std::string description = result->getString("description");
        uint32_t reqLevel = result->getNumber<uint32_t>("required_level");
        bool isStarting = result->getNumber<uint8_t>("is_starting_node") == 1;
        
        // Cria o nó e adiciona ao mapa
        nodes.emplace(std::piecewise_construct,
                     std::forward_as_tuple(id),
                     std::forward_as_tuple(id, name, type, posX, posY, spriteId, description, reqLevel, isStarting));
        
    } while (result->next());
    
    return true;
}

bool SkillTreeManager::loadConnections() {
    Database& db = Database::getInstance();
    
    std::ostringstream query;
    query << "SELECT `node_id_from`, `node_id_to` FROM `skilltree_connections`";
    
    DBResult_ptr result = db.storeQuery(query.str());
    if (!result) {
        return true; // Sem conexões, mas não é erro
    }
    
    do {
        uint32_t nodeFrom = result->getNumber<uint32_t>("node_id_from");
        uint32_t nodeTo = result->getNumber<uint32_t>("node_id_to");
        
        // Adiciona a conexão nos dois sentidos (bidirecional)
        SkillTreeNode* fromNode = getNode(nodeFrom);
        SkillTreeNode* toNode = getNode(nodeTo);
        
        if (fromNode && toNode) {
            fromNode->addConnection(nodeTo);
            toNode->addConnection(nodeFrom);
        }
        
    } while (result->next());
    
    return true;
}

bool SkillTreeManager::loadBonuses() {
    Database& db = Database::getInstance();
    
    std::ostringstream query;
    query << "SELECT `node_id`, `bonus_type`, `bonus_value`, `is_percentage`, `vocation_id` "
          << "FROM `skilltree_node_bonuses`";
    
    DBResult_ptr result = db.storeQuery(query.str());
    if (!result) {
        return true; // Sem bônus, mas não é erro
    }
    
    do {
        uint32_t nodeId = result->getNumber<uint32_t>("node_id");
        std::string bonusTypeStr = result->getString("bonus_type");
        int32_t bonusValue = result->getNumber<int32_t>("bonus_value");
        bool isPercentage = result->getNumber<uint8_t>("is_percentage") == 1;
        uint16_t vocationId = result->getNumber<uint16_t>("vocation_id");
        
        // Converte string para enum (você pode melhorar isso com um map)
        SkillTreeBonusType bonusType = stringToBonusType(bonusTypeStr);
        
        // Adiciona o bônus ao nó
        SkillTreeNode* node = getNode(nodeId);
        if (node) {
            node->addBonus(SkillTreeBonus(bonusType, bonusValue, isPercentage, vocationId));
        }
        
    } while (result->next());
    
    return true;
}

// Função auxiliar para converter string em enum
SkillTreeBonusType SkillTreeManager::stringToBonusType(const std::string& str) {
    // Mapeamento string -> enum
    static const std::map<std::string, SkillTreeBonusType> bonusMap = {
        // Atributos base
        {"strength", BONUS_STRENGTH},
        {"dexterity", BONUS_DEXTERITY},
        {"intelligence", BONUS_INTELLIGENCE},
        {"vitality", BONUS_VITALITY},
        {"wisdom", BONUS_WISDOM},
        {"spirit", BONUS_SPIRIT},
        {"speed", BONUS_SPEED},
        
        // Absorções
        {"absorb_fire", BONUS_ABSORB_FIRE},
        {"absorb_arcane", BONUS_ABSORB_ARCANE},
        {"absorb_water", BONUS_ABSORB_WATER},
        {"absorb_ice", BONUS_ABSORB_ICE},
        {"absorb_holy", BONUS_ABSORB_HOLY},
        {"absorb_death", BONUS_ABSORB_DEATH},
        {"absorb_drown", BONUS_ABSORB_DROWN},
        {"absorb_lifedrain", BONUS_ABSORB_LIFEDRAIN},
        {"absorb_manadrain", BONUS_ABSORB_MANADRAIN},
        {"absorb_earth", BONUS_ABSORB_EARTH},
        {"absorb_physical", BONUS_ABSORB_PHYSICAL},
        
        // Dano incremental
        {"damage_fire", BONUS_DAMAGE_FIRE},
        {"damage_arcane", BONUS_DAMAGE_ARCANE},
        {"damage_water", BONUS_DAMAGE_WATER},
        {"damage_ice", BONUS_DAMAGE_ICE},
        {"damage_holy", BONUS_DAMAGE_HOLY},
        {"damage_death", BONUS_DAMAGE_DEATH},
        {"damage_drown", BONUS_DAMAGE_DROWN},
        {"damage_lifedrain", BONUS_DAMAGE_LIFEDRAIN},
        {"damage_manadrain", BONUS_DAMAGE_MANADRAIN},
        {"damage_earth", BONUS_DAMAGE_EARTH},
        {"damage_physical", BONUS_DAMAGE_PHYSICAL},
        
        // Bônus especiais
        {"rarity_loot", BONUS_RARITY_LOOT},
        {"exp_global", BONUS_EXP_GLOBAL},
        {"loot_global", BONUS_LOOT_GLOBAL},
        
        // Combat
        {"critical_chance", BONUS_CRITICAL_CHANCE},
        {"critical_amount", BONUS_CRITICAL_AMOUNT},
        {"life_leech_chance", BONUS_LIFE_LEECH_CHANCE},
        {"life_leech_amount", BONUS_LIFE_LEECH_AMOUNT},
        {"mana_leech_chance", BONUS_MANA_LEECH_CHANCE},
        {"mana_leech_amount", BONUS_MANA_LEECH_AMOUNT},
        
        // Skills
        {"skill_sword", BONUS_SKILL_SWORD},
        {"skill_club", BONUS_SKILL_CLUB},
        {"skill_axe", BONUS_SKILL_AXE},
        {"skill_distance", BONUS_SKILL_DISTANCE},
        {"skill_shielding", BONUS_SKILL_SHIELDING},
        {"skill_magic", BONUS_SKILL_MAGIC},
        
        // Profissões
        {"skill_crafting", BONUS_SKILL_CRAFTING},
        {"skill_herbalist", BONUS_SKILL_HERBALIST},
        {"skill_jewelsmith", BONUS_SKILL_JEWELSMITH},
        {"skill_armorsmith", BONUS_SKILL_ARMORSMITH},
        {"skill_weaponsmith", BONUS_SKILL_WEAPONSMITH},
        {"skill_woodcutting", BONUS_SKILL_WOODCUTTING},
        {"skill_mining", BONUS_SKILL_MINING}
    };
    
    auto it = bonusMap.find(str);
    if (it != bonusMap.end()) {
        return it->second;
    }
    
    return BONUS_STRENGTH; // Default
}