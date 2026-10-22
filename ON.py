import sys
import socket
import struct
import select

def create_network_matrix(filepath):
    upper_triangle = []
    with open(filepath, 'r') as f:
        for line in f:
            line = line.strip() 
            
            if not line or line.startswith('#'):
                continue

            costs = [int(x) for x in line.split()]
            upper_triangle.append(costs)

    num_nodes = len(upper_triangle) + 1
    for i in range(len(upper_triangle)):
        if len(upper_triangle[i]) != num_nodes - 1 - i or any(c == 0 or c < -1 for c in upper_triangle[i]):
            raise ValueError("bad config file!")
    matrix = [[-1 for _ in range(num_nodes)] for _ in range(num_nodes)]
    
    for i in range(num_nodes):
        matrix[i][i] = 0
        
    for i in range(len(upper_triangle)):
        for j in range(len(upper_triangle[i])):
            cost = upper_triangle[i][j]
            col = i + 1 + j
            matrix[i][col] = cost
            matrix[col][i] = cost 
            
    return matrix

def build_link_state(node_id, node_map, matrix):
    i = ord(node_id) - ord('A')
    tuples = [i] + [j for j in range(len(matrix)) if matrix[i][j] > 0]
    msg = struct.pack('!B', len(tuples))
    for j in tuples:
        ip_addr, udp_port = node_map[chr(ord('A') + j)]
        msg += struct.pack('!c4sHI', chr(ord('A') + j).encode(), socket.inet_aton(ip_addr), udp_port, matrix[i][j])
    return msg

if __name__ == "__main__":
    if len(sys.argv) != 2:
        print("Config file?\n")
        sys.exit(1)
        
    network_matrix = create_network_matrix(sys.argv[1])
    
    for row in network_matrix:
        print("\t".join(str(val) for val in row))

    # bind ON to tcp port 5000
    server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    server_socket.bind(('', 5000))
    server_socket.listen(26)

    num_nodes = len(network_matrix)
    node_map = {}
    client_sockets = {}
    seen_nodes = 0

    # receive connect messages from nodes
    while (seen_nodes < num_nodes):
        client_socket, addr = server_socket.accept()
        data = client_socket.recv(6)
        if len(data) < 6:
            print("received data is too short.")
            continue
        ip_addr = socket.inet_ntoa(data[:4])
        udp_port = int.from_bytes(data[4:6], byteorder='big')
        node_id = chr(ord('A') + seen_nodes)
        node_map[node_id] = (ip_addr, udp_port)
        client_sockets[node_id] = client_socket
        seen_nodes += 1
        
        print(f"received connection from {ip_addr}:{udp_port}")

    #send link state message to all nodes
    for node_id, active_socket in client_sockets.items():
        active_socket.sendall(build_link_state(node_id, node_map, network_matrix))
        print(f"successfully sent LINK-STATE to Node {node_id}")

    while True:
        readable, _, _ = select.select([server_socket] + list(client_sockets.values()), [], [], 1)

        #received connection from island node
        if server_socket in readable:
            client_socket, addr = server_socket.accept()
            data = client_socket.recv(6)
            if len(data) < 6:
                print("received data is too short")
            else:
                ip_addr = socket.inet_ntoa(data[:4])
                udp_port = int.from_bytes(data[4:6], byteorder='big')
                node_id = chr(ord('A') + seen_nodes)
                client_sockets[node_id] = client_socket
                seen_nodes += 1
                client_socket.sendall(struct.pack('!Bc4sHI', 1, node_id.encode(), data[:4], udp_port, 0))
                print(f"received connection from {ip_addr}:{udp_port}, node {node_id} is an island")

        # remove disconnected nodes if any
        for node_id, active_socket in list(client_sockets.items()):
            if active_socket not in readable:
                continue
            try:
                data = active_socket.recv(1)
            except ConnectionError:
                data = b''
            if not data:
                print(f"node {node_id} disconnected")
                active_socket.close()
                del client_sockets[node_id]

        #checking for changes in config file
        try:
            new_matrix = create_network_matrix(sys.argv[1])
        except (OSError, ValueError):
            print("config file not readable")
            continue
        if len(new_matrix) != num_nodes:
            print("number of nodes changed")
        #config file changed
        elif new_matrix != network_matrix:
            network_matrix = new_matrix
            print("config changed")
            for row in network_matrix:
                print("\t".join(str(val) for val in row))
            for node_id in node_map:
                if node_id in client_sockets:
                    client_sockets[node_id].sendall(build_link_state(node_id, node_map, network_matrix))
                    print(f"successfully sent LINK-STATE to Node {node_id}")
        

    