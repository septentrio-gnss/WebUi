#ifndef __SEPTENTRIO_STRUCTS_H__
#define __SEPTENTRIO_STRUCTS_H__

#include <Arduino.h>

// La taille maximale d'un bloc SBF que nous prévoyons de traiter.
// 512 octets est suffisant pour la plupart des blocs PVT, mais peut
// être augmenté si vous traitez des blocs de mesures brutes très volumineux.
// (MAX_SBFSIZE est 65532, mais c'est trop pour la RAM d'un ESP32)
constexpr size_t SBF_BUFFER_MAX_SIZE = 512;
constexpr size_t NMEA_BUFFER_MAX_SIZE = 128 ; 


// Valeurs "Do-Not-Use" (invalides) standards
constexpr uint32_t SBF_U4_INVALID = 0xFFFFFFFF;
constexpr uint16_t SBF_U2_INVALID = 0xFFFF;
constexpr float    SBF_F4_INVALID = -2e10F;
constexpr double   SBF_F8_INVALID = -2e10;

/**
 * @brief Buffer pour un message SBF complet et validé.
 * * Ce buffer contient le message SBF *complet*, y compris l'en-tête de 8 octets
 * (commençant par '$@') et le payload.
 * Les fonctions de conversion (u2Conv, f4Conv, etc.) s'attendent à ce que
 * SBFBuffer.data[0] == '$' et SBFBuffer.data[1] == '@'.
 * * Les offsets mentionnés dans la documentation PDF (par ex. TOW à l'offset 8)
 * s'appliquent directement à ce buffer `data`.
 */
struct sbfBuffer_t {
    // Le buffer de données brutes contenant le message complet ($@...payload)
    uint8_t data[SBF_BUFFER_MAX_SIZE];
    
    // La taille totale du message en octets (valeur du champ 'Length')
    uint16_t msgSize;
    
    // === Champs extraits pour un accès facile ===

    // L'ID du bloc SBF (bits 0-12 du champ ID)
    uint16_t block_id;
    
    // La révision du bloc (bits 13-15 du champ ID)
    uint16_t revision;
    
    // Horodatage (TOW et WNc), lu depuis les offsets 8 et 12
    uint32_t tow;
    uint16_t wnc;
    
    // Vrai si le TOW et le WNc ne sont pas des valeurs "Do-Not-Use"
    bool timeValid;
};


struct nmeaBuffer_t {
    // Le buffer de données brutes contenant la phrase NMEA complète
    char data[NMEA_BUFFER_MAX_SIZE];
    
    // La taille totale de la phrase en octets
    uint16_t msgSize;
    
    // Vrai si la phrase NMEA est valide (checksum correcte)
    bool isValid;

    char talkerID[3]; // Ex: "GP", "GL", etc.
    char messageID[4]; // Ex: "GGA", "RMC", etc.
};
#endif // __SEPTENTRIO_STRUCTS_H__