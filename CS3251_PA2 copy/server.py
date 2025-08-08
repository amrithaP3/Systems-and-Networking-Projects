import argparse
import socket
import threading
import time
import sys

# Global variables
HOST = "127.0.0.1"
SIZE = 1024
FORMAT = "utf-8"
DISCONNECT_MESSAGE = ":Exit"

clients = []
usernames = []

def broadcast(msg, sender):
    for client in clients:
        if client != sender:
            client.send(msg.encode(FORMAT))

def client_handler(client):
    connected = True
    while connected:
        try:
            # Receive the message from the client
            msg = client.recv(SIZE).decode(FORMAT)

            if msg == DISCONNECT_MESSAGE:
                # Remove the client from the list
                if client in clients:
                    index = clients.index(client)
                    clients.remove(client)

                    # Get the username of the client
                    username = usernames[index]
                    usernames.remove(username)

                    # Print info on server-side
                    print(f"{username} left the chatroom")
                    sys.stdout.flush()

                    # Send a message to all clients that a client has left
                    broadcast(f"{username} left the chatroom", client)
                client.close()
                return

            if ":dm" in msg:
                direct_chat = msg.split(" ", 2)
                receiver_username = direct_chat[1]
                message = direct_chat[2]
                msg = message

                # Send the message to the receiver (direct chat)
                for i in range(len(usernames)):
                    if usernames[i] == receiver_username:
                        clients[i].send(f"{usernames[clients.index(client)]}: {msg}".encode(FORMAT))

                        print(f"{usernames[clients.index(client)]} to {receiver_username}: {msg}")
                        sys.stdout.flush()
                        break
            else:
                # Broadcast the message to all clients and print info on server-side
                print(f"{usernames[clients.index(client)]}: {msg}")
                sys.stdout.flush()

                broadcast(f"{usernames[clients.index(client)]}: {msg}", client)
        except:
            # Remove the client from the list
            if client in clients:
                index = clients.index(client)
                clients.remove(client)

                # Get the username of the client
                username = usernames[index]
                usernames.remove(username)

                broadcast(f"{username} left the chatroom", client)
            client.close()
            break
    
    # Close the client
    client.close()

def main(passcode, port):
    print(f"Server started on port {port}. Accepting connections")
    sys.stdout.flush()

    # Create a TCP socket
    server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

    # Bind the socket to the address
    ADDR = (HOST, port)
    server.bind(ADDR)

    # Listen for incoming connections from clients
    server.listen()
    
    while True:
        # Accept the connection from the client
        client, addr = server.accept()

        # Receive the credentials from the client
        credentials = client.recv(SIZE).decode(FORMAT)

        username, client_passcode = credentials.split(":")
        if client_passcode != passcode:
            client.send("Incorrect passcode".encode(FORMAT))
            client.close()
            continue
        else:
            client.send("Welcome to the chatroom!".encode(FORMAT))
            usernames.append(username)
            clients.append(client)

        # Send a message to all clients that a new client has joined
        msg = f"{username} joined the chatroom"
        print(msg)
        sys.stdout.flush()
        broadcast(msg, client)

        # Start a new thread to handle the client
        thread = threading.Thread(target=client_handler, args=(client,))
        thread.start()

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Chatroom Server")

    parser.add_argument("-start", action="store_true", help="Start the server")
    parser.add_argument("-port", type=int, required=True, help="Port to listen on")
    parser.add_argument("-passcode", required=True, help="Chatroom passcode")

    args = parser.parse_args()
    main(args.passcode, args.port)