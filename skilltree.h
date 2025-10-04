/**
 * The Forgotten Server - a free and open-source MMORPG server emulator
 * Copyright (C) 2019  Mark Samman <mark.samman@gmail.com>
 *
 * SKILL TREE SYSTEM - Path of Exile Style
 * Sistema de árvore de talentos com nós conectados
 */

#ifndef FS_SKILLTREE_H
#define FS_SKILLTREE_H

#include "player.h"
#include "database.h"
#include <map>
#include <vector>
#include <set>
#include <string>

// Forward declarations
class Player;

// =====================================================
// ENUMS E CONSTANTES
// =====================================================

// Tipos de nós na skill tree
enum SkillTreeNodeType : uint8_t {
    SKILLTREE_NODE_SMALL = 0,      // Nó pequeno (passivo simples)
    SKILLTREE_NODE_MEDIUM = 1,     // Nó médio (notável)
    SKILLTREE_NODE_LARGE = 2       // Nó grande (keystone)
};

// Tipos de bônus disponíveis
enum SkillTreeBonusType : uint8_t {
    // Atributos base
    BONUS_STRENGTH = 0,
    BONUS_DEXTERITY = 1,
    BONUS_INTELLIGENCE = 2,
    BONUS_VITALITY = 3,
    BONUS_WISDOM = 4,
    BONUS_SPIRIT = 5,
    BONUS_SPEED = 6,
    
    // Absorções (resistências)
    BONUS_ABSORB_FIRE = 10,
    BONUS_ABSORB_ARCANE = 11,
    BONUS_ABSORB_WATER = 12,
    BONUS_ABSORB_ICE = 13,
    BONUS_ABSORB_HOLY = 14,
    BONUS_ABSORB_DEATH = 15,
    BONUS_ABSORB_DROWN = 16,
    BONUS_ABSORB_LIFEDRAIN = 17,
    BONUS_ABSORB_MANADRAIN = 18,
    BONUS_ABSORB_EARTH = 19,
    BONUS_ABSORB_PHYSICAL = 20,
    
    // Dano incremental
    BONUS_DAMAGE_FIRE = 30,
    BONUS_DAMAGE_ARCANE = 31,
    BONUS_DAMAGE_WATER = 32,
    BONUS_DAMAGE_ICE = 33,
    BONUS_DAMAGE_HOLY = 34,
    BONUS_DAMAGE_DEATH = 35,
    BONUS_DAMAGE_DROWN = 36,
    BONUS_DAMAGE_LIFEDRAIN = 37,
    BONUS_DAMAGE_MANADRAIN = 38,
    BONUS_DAMAGE_EARTH = 39,
    BONUS_DAMAGE_PHYSICAL = 40,
    
    // Bônus especiais
    BONUS_RARITY_LOOT = 50,
    BONUS_EXP_GLOBAL = 51,
    BONUS_LOOT_GLOBAL = 52,
    
    // Combat
    BONUS_CRITICAL_CHANCE = 60,
    BONUS_CRITICAL_AMOUNT = 61,
    BONUS_LIFE_LEECH_CHANCE = 62,
    BONUS_LIFE_LEECH_AMOUNT = 63,
    BONUS_MANA_LEECH_CHANCE = 64,
    BONUS_MANA_LEECH_AMOUNT = 65,
    
    // Skills
    BONUS_SKILL_SWORD = 70,
    BONUS_SKILL_CLUB = 71,
    BONUS_SKILL_AXE = 72,
    BONUS_SKILL_DISTANCE = 73,
    BONUS_SKILL_SHIELDING = 74,
    BONUS_SKILL_MAGIC = 75,
    
    // Profissões
    BONUS_SKILL_CRAFTING = 80,
    BONUS_SKILL_HERBALIST = 81,
    BONUS_SKILL_JEWELSMITH = 82,
    BONUS_SKILL_ARMORSMITH = 83,
    BONUS_SKILL_WEAPONSMITH = 84,
    BONUS_SKILL_WOODCUTTING = 85,
    BONUS_SKILL_MINING = 86
};

// Constantes do sistema
constexpr uint32_t SKILLTREE_MIN_LEVEL = 100;     // Nível mínimo para começar a ganhar pontos
constexpr uint32_t SKILLTREE_MAX_LEVEL = 250;     // Nível máximo que dá pontos
constexpr uint32_t SKILLTREE_MAX_POINTS = 150;    // Máximo de pontos possíveis
constexpr uint32_t SKILLTREE_RESET_COST_PER_LEVEL = 250; // Custo em gold coins por level para resetar

// =====================================================
// CLASSE: SkillTreeBonus
// Representa um bônus individual de um nó
// =====================================================
class SkillTreeBonus {
public:
    SkillTreeBonus(SkillTreeBonusType type, int32_t value, bool isPercentage, uint16_t vocationId = 0)
        : bonusType(type), bonusValue(value), isPercentage(isPercentage), vocationId(vocationId) {}
    
    SkillTreeBonusType getBonusType() const { return bonusType; }
    int32_t getBonusValue() const { return bonusValue; }
    bool isPercentageBonus() const { return isPercentage; }
    uint16_t getVocationId() const { return vocationId; }
    
    // Converte o tipo de bônus para string (para debug/logs)
    static std::string bonusTypeToString(SkillTreeBonusType type);
    
private:
    SkillTreeBonusType bonusType;  // Tipo do bônus
    int32_t bonusValue;             // Valor do bônus
    bool isPercentage;              // true = porcentagem, false = valor fixo
    uint16_t vocationId;            // 0 = todas vocações, ou ID específico
};

// =====================================================
// CLASSE: SkillTreeNode
// Representa um nó individual na skill tree
// =====================================================
class SkillTreeNode {
public:
    SkillTreeNode(uint32_t id, const std::string& name, SkillTreeNodeType type, 
                  float x, float y, uint32_t spriteId, const std::string& desc, 
                  uint32_t reqLevel, bool isStarting)
        : nodeId(id), nodeName(name), nodeType(type), positionX(x), positionY(y),
          spriteId(spriteId), description(desc), requiredLevel(reqLevel), isStartingNode(isStarting) {}
    
    // Getters
    uint32_t getNodeId() const { return nodeId; }
    std::string getNodeName() const { return nodeName; }
    SkillTreeNodeType getNodeType() const { return nodeType; }
    float getPositionX() const { return positionX; }
    float getPositionY() const { return positionY; }
    uint32_t getSpriteId() const { return spriteId; }
    std::string getDescription() const { return description; }
    uint32_t getRequiredLevel() const { return requiredLevel; }
    bool isStarting() const { return isStartingNode; }
    
    // Gerenciamento de conexões
    void addConnection(uint32_t nodeId) { connectedNodes.insert(nodeId); }
    const std::set<uint32_t>& getConnections() const { return connectedNodes; }
    bool isConnectedTo(uint32_t nodeId) const { return connectedNodes.find(nodeId) != connectedNodes.end(); }
    
    // Gerenciamento de bônus
    void addBonus(const SkillTreeBonus& bonus) { bonuses.push_back(bonus); }
    const std::vector<SkillTreeBonus>& getBonuses() const { return bonuses; }
    
    // Retorna apenas os bônus válidos para uma vocação específica
    std::vector<SkillTreeBonus> getBonusesForVocation(uint16_t vocationId) const;
    
private:
    uint32_t nodeId;                        // ID único do nó
    std::string nodeName;                   // Nome do nó
    SkillTreeNodeType nodeType;             // Tipo (pequeno, médio, grande)
    float positionX;                        // Posição X no grid visual
    float positionY;                        // Posição Y no grid visual
    uint32_t spriteId;                      // ID do sprite para renderização
    std::string description;                // Descrição do nó
    uint32_t requiredLevel;                 // Nível mínimo requerido
    bool isStartingNode;                    // true se é um nó inicial
    
    std::set<uint32_t> connectedNodes;      // IDs dos nós conectados a este
    std::vector<SkillTreeBonus> bonuses;    // Lista de bônus deste nó
};

// =====================================================
// CLASSE: PlayerSkillTree
// Gerencia a skill tree de um player específico
// =====================================================
class PlayerSkillTree {
public:
    PlayerSkillTree(Player* player) : player(player), availablePoints(0), totalPointsEarned(0), pointsSpent(0) {}
    
    // Carregar do banco de dados
    bool loadFromDatabase();
    
    // Salvar no banco de dados
    bool saveToDatabase();
    
    // Gerenciamento de pontos
    uint32_t getAvailablePoints() const { return availablePoints; }
    uint32_t getTotalPointsEarned() const { return totalPointsEarned; }
    uint32_t getPointsSpent() const { return pointsSpent; }
    
    // Calcula quantos pontos o player deveria ter baseado no level
    void updatePointsBasedOnLevel();
    
    // Gerenciamento de nós
    bool hasNodeAllocated(uint32_t nodeId) const { return allocatedNodes.find(nodeId) != allocatedNodes.end(); }
    const std::set<uint32_t>& getAllocatedNodes() const { return allocatedNodes; }
    
    // Aloca um nó (retorna true se sucesso)
    bool allocateNode(uint32_t nodeId);
    
    // Desaloca um nó (retorna true se sucesso)
    bool deallocateNode(uint32_t nodeId);
    
    // Reseta toda a árvore (retorna custo em gold)
    uint64_t resetTree();
    
    // Verifica se um nó pode ser alocado (tem pontos, está conectado, etc)
    bool canAllocateNode(uint32_t nodeId, std::string& errorMessage) const;
    
    // Verifica se um nó está conectado a algum nó já alocado
    bool isNodeReachable(uint32_t nodeId) const;
    
    // Calcula todos os bônus ativos do player
    void calculateBonuses();
    
    // Aplica os bônus ao player (chamado ao logar ou alocar/desalocar nó)
    void applyBonusesToPlayer();
    
    // Envia a skill tree para o client
    void sendToClient();
    
private:
    Player* player;                         // Ponteiro para o player
    uint32_t availablePoints;               // Pontos disponíveis
    uint32_t totalPointsEarned;             // Total de pontos ganhos
    uint32_t pointsSpent;                   // Pontos gastos
    std::set<uint32_t> allocatedNodes;      // IDs dos nós alocados
    
    // Cache de bônus calculados (evita recalcular sempre)
    std::map<SkillTreeBonusType, int32_t> cachedBonuses;
};

// =====================================================
// CLASSE: SkillTreeManager (Singleton)
// Gerencia todo o sistema de skill tree globalmente
// =====================================================
class SkillTreeManager {
public:
    // Singleton
    static SkillTreeManager& getInstance() {
        static SkillTreeManager instance;
        return instance;
    }
    
    // Previne cópia
    SkillTreeManager(const SkillTreeManager&) = delete;
    SkillTreeManager& operator=(const SkillTreeManager&) = delete;
    
    // Inicialização
    bool loadFromDatabase();
    void clear();
    
    // Getters
    SkillTreeNode* getNode(uint32_t nodeId);
    const std::map<uint32_t, SkillTreeNode>& getAllNodes() const { return nodes; }
    
    // Validações
    bool nodeExists(uint32_t nodeId) const { return nodes.find(nodeId) != nodes.end(); }
    
    // Busca nós por tipo
    std::vector<SkillTreeNode*> getNodesByType(SkillTreeNodeType type);
    
    // Busca nós iniciais (para cada vocação)
    std::vector<SkillTreeNode*> getStartingNodes();
    
    // Debug/Admin
    void reloadFromDatabase();
    std::string getStatistics() const;
    
private:
    SkillTreeManager() = default;
    
    std::map<uint32_t, SkillTreeNode> nodes;  // Todos os nós da árvore (ID -> Node)
    
    // Carrega nós do banco
    bool loadNodes();
    
    // Carrega conexões do banco
    bool loadConnections();
    
    // Carrega bônus do banco
    bool loadBonuses();
    
    // Converte string para SkillTreeBonusType
    SkillTreeBonusType stringToBonusType(const std::string& str);
};

#endif // FS_SKILLTREE_H