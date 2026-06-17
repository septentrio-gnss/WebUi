#ifndef SEPTENTRIO_ARDUINO_DRIVER_H
#define SEPTENTRIO_ARDUINO_DRIVER_H

#include <Arduino.h>
#include <Stream.h>
#include <WiFiClient.h> // Ajouté pour la classe NTRIP
#include "septentrio_structs.h"

// --- Classe de parsing GNSS (SBF & NMEA) ---
class SEPTENTRIO_GNSS {

public:
    // Buffers publics pour les derniers messages valides
    sbfBuffer_t SBFBuffer;
    nmeaBuffer_t NMEABuffer;

    SEPTENTRIO_GNSS();

    /**
     * @brief Initialise le driver avec le port série.
     * @param serialPort Le port Stream (ex: Serial2) connecté au récepteur.
     */
    void begin(Stream &serialPort);

    /**
     * @brief (NOUVEAU) Vérifie si le parseur SBF est en train de lire un message.
     * @return 'true' si le parseur a trouvé '$@' et attend la fin du bloc.
     */
    bool isParsingSbf();
    
    /**
     * @brief (NOUVEAU) Vérifie si le parseur NMEA est en train de lire un message.
     * @return 'true' si le parseur a trouvé '$' et attend la fin du bloc.
     */
    bool isParsingNmea();
    
    /**
     * @brief Active les messages de débogage.
     * @param debugPort Le port Stream pour afficher les logs (ex: Serial).
     */
    void enableDebug(Stream &debugPort);

    /**
     * @brief Désactive les messages de débogage.
     */
    void disableDebug();

    /**
     * @brief Analyse l'octet entrant pour un message SBF.
     * @return 'true' si un bloc SBF complet et valide est prêt dans SBFBuffer.
     */
    bool readSbf(uint8_t incomingByte);

    /**
     * @brief (NOUVEAU) Analyse l'octet entrant pour un message NMEA.
     * @return 'true' si une phrase NMEA complète et valide est prête dans NMEABuffer.
     */
    bool readNmea(uint8_t incomingByte);

    // --- Fonctions d'aide à la conversion SBF ---
    uint16_t u2Conv(const sbfBuffer_t *buffer, uint16_t offset);
    uint32_t u4Conv(const sbfBuffer_t *buffer, uint16_t offset);
    uint64_t u8Conv(const sbfBuffer_t *buffer, uint16_t offset);
    int16_t  i2Conv(const sbfBuffer_t *buffer, uint16_t offset);
    int32_t  i4Conv(const sbfBuffer_t *buffer, uint16_t offset);
    float    f4Conv(const sbfBuffer_t *buffer, uint16_t offset);
    double   f8Conv(const sbfBuffer_t *buffer, uint16_t offset);

private:
    Stream* _serialPort; // Port série du récepteur
    Stream* _debugPort;
    bool _printDebug = false;

    // --- Machine à états SBF ---
    enum class SbfParseState { WAITING_SYNC1, WAITING_SYNC2, READING_HEADER, READING_PAYLOAD };
    SbfParseState _sbfState = SbfParseState::WAITING_SYNC1;
    uint8_t _sbf_temp_buffer[SBF_BUFFER_MAX_SIZE];
    uint16_t _sbf_offset = 0;
    uint16_t _sbf_expected_length = 0;
    uint16_t compute_sbf_crc(const uint8_t* data, size_t length);
    void fillSBFProperties(sbfBuffer_t *buffer);

    // --- Machine à états NMEA (NOUVEAU) ---
    enum class NmeaParseState { WAITING_SYNC, READING_PAYLOAD, READING_CHECKSUM1, READING_CHECKSUM2 };
    NmeaParseState _nmeaState = NmeaParseState::WAITING_SYNC;
    char _nmea_buffer[NMEA_BUFFER_MAX_SIZE];
    uint16_t _nmea_offset = 0;
    uint8_t _nmea_checksum = 0;
    uint8_t _nmea_received_checksum = 0;
    uint8_t hex_char_to_byte(char c);
    void fillNMEAProperties(nmeaBuffer_t *buffer);

    template<typename T>
    T convertFromBuffer(const sbfBuffer_t* buffer, uint16_t offset);
};


// --- Classe Client NTRIP (NOUVEAU / REFACTORISÉ) ---
class SEPTENTRIO_NTRIP {

public:
    SEPTENTRIO_NTRIP();

    /**
     * @brief Initialise le client NTRIP.
     * @param receiverPort Le port Stream (ex: Serial2) vers lequel écrire les données RTCM reçues.
     */
    void begin(Stream &receiverPort);

    /**
     * @brief Tente de se connecter au Caster NTRIP.
     * @return 'true' si la connexion (HTTP 200 OK) est réussie.
     */
    bool connect(String host, int port, String mount, String user, String pass);

    /**
     * @brief Se déconnecte du Caster NTRIP.
     */
    void disconnect();

    /**
     * @brief Fonction à appeler dans la boucle principale.
     * Gère la réception des données RTCM et l'envoi périodique du GGA.
     * @return Le nombre d'octets RTCM reçus et transférés au récepteur.
     */
    size_t loop();

    /**
     * @brief Met à jour la phrase GGA à envoyer périodiquement au caster.
     * @param ggaSentence La phrase $GPGGA complète, avec checksum et \r\n.
     */
    void updateGGA(String ggaSentence);

    /**
     * @brief Vérifie si le client est actuellement connecté.
     */
    bool isConnected();

    /**
     * @brief Active les messages de débogage.
     * @param debugPort Le port Stream pour afficher les logs (ex: Serial).
     */
    void enableDebug(Stream &debugPort);

private:
    WiFiClient _ntripClient;
    Stream* _receiverPort; // Port série du récepteur (Serial2)
    Stream* _debugPort;
    bool _printDebug = false;
    
    String _ggaSentence = "";
    unsigned long _lastGgaSend = 0;
    bool _ntripConnected = false;

    String toBase64(String s);
};

#endif // SEPTENTRIO_ARDUINO_DRIVER_H