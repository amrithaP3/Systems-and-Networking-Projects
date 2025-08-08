import argparse
import socket
import sys
import threading
import time

# Global variables
# should size be 100 for 100 chars?
SIZE = 1024
FORMAT = "utf-8"
DISCONNECT_MESSAGE = ":Exit"

def dynamic_shortcuts(msg):
    if msg == ":)":
        return "[Feeling Joyful]"
    elif msg == ":(":
        return "[Feeling Unhappy]"
    elif msg == ":mytime":
        return time.strftime("%Y %b %d %H:%M:%S %a")
    elif msg == ":+1hr":
        return time.strftime("%Y %b %d %H:%M:%S %a", time.localtime(time.time() + 3600))
    else:
        return msg
    
def receive(client):
    while True:
        # Receive the message from the server
        try:
            msg = client.recv(SIZE).decode(FORMAT)
            if not msg:
                break
            print(msg)
            sys.stdout.flush()
        except ConnectionResetError:
            break
        except OSError:
            break

def send(client, username):
    connected = True
    while connected:
        # Send a message to the server
        msg = input("")

        if msg == DISCONNECT_MESSAGE:
            connected = False
            client.send(msg.encode(FORMAT))
            client.close()
            return
        else:
            msg = dynamic_shortcuts(msg)

        client.send(msg.encode(FORMAT))

def main(host, port, addr, username, passcode):
    client = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    client.connect(addr)

    # Send the username and passcode to the server
    credentials = f"{username}:{passcode}"
    client.send(credentials.encode(FORMAT))

    msg = client.recv(SIZE).decode(FORMAT)
    if msg == "Invalid credentials":
        print("Invalid credentials")
        sys.stdout.flush()
        client.close()
        return
    else:
        print(f"Connected to {host} on port {port}")
        sys.stdout.flush()

    # Thread to receive messages from the server
    receive_thread = threading.Thread(target=receive, args=(client,), daemon=True)
    receive_thread.start()

    # Send messages to the server
    send(client, username)
   

if __name__ == "__main__":
    parser = argparse.ArgumentParser(prog="Client.py", description="Chatroom Client")
    
    parser.add_argument("-join", action="store_true", help="Joining the chatroom")
    parser.add_argument("-host", required=True, help="Server hostname")
    parser.add_argument("-port", type=int, required=True, help="Server port")
    parser.add_argument("-username", required=True, help="Client username")
    parser.add_argument("-passcode", required=True, help="Chatroom passcode")

    args = parser.parse_args()

    # Check if the username is less than 8 characters
    if len(args.username) > 8:
        print("Username must be less than 8 characters")
        sys.stdout.flush()
        sys.exit(1)
    
    # Check if the passcode is less than 5 characters
    if len(args.passcode) > 5:
        print("Incorrect passcode")
        sys.stdout.flush()
        sys.exit(1)

    main(args.host, args.port, (args.host, args.port), args.username, args.passcode)
