import socket
import time
import sys

# --- CONFIGURATION ---
# 1. Remplacez par l'adresse IP de votre ESP32 (vue dans le Moniteur Série)
ESP32_IP = "dualy.local"  

# 2. Port TCP configuré dans le code de l'ESP32
ESP32_PORT = 8888

# 3. Remplacez par le nom exact de votre fichier de log NMEA
NMEA_FILE = r"C:\Users\sarie\OneDrive\Documents\PlatformIO\Projects\ESP32 dualy\test\log_0000.nmea"

# 4. Temps d'attente entre l'envoi de chaque ligne (en secondes)
#    0.1 = 10 lignes par seconde (10 Hz)
#    1.0 = 1 ligne par seconde (1 Hz)
DELAY = 0.1 

try:
    with open(NMEA_FILE, 'r') as f:
        nmea_lines = f.readlines()
    print(f"Successfully loaded {len(nmea_lines)} lines from {NMEA_FILE}.")
except FileNotFoundError:
    print(f"ERROR: The file '{NMEA_FILE}' was not found.")
    print("Please make sure this script is in the same folder as your log file.")
    sys.exit(1)


# Boucle infinie pour pouvoir relancer le stream sans redémarrer le script
while True: 
    try:
        print(f"\nConnecting to ESP32 at {ESP32_IP}:{ESP32_PORT}...")
        # Crée une connexion socket TCP/IP
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
            s.connect((ESP32_IP, ESP32_PORT))
            print("Connection successful! Streaming NMEA data...")
            
            # Envoie chaque ligne du fichier
            for line in nmea_lines:
                line = line.strip() # Nettoie la ligne (enlève les espaces et sauts de ligne inutiles)
                if line: # S'assure que la ligne n'est pas vide
                    # Encodage en UTF-8 et ajout d'un saut de ligne pour que l'ESP32 le reçoive correctement
                    s.sendall(line.encode('utf-8') + b'\n')
                    print(f"Sent: {line}")
                    time.sleep(DELAY) # Attend le délai configuré

    except ConnectionRefusedError:
        print("Connection refused. Is the ESP32 running and on the same WiFi network?")
    except KeyboardInterrupt:
        print("\nStream stopped by user.")
        sys.exit(0)
    except Exception as e:
        print(f"An error occurred: {e}")

    print("Stream finished or connection lost. Restarting in 5 seconds...")
    time.sleep(5)