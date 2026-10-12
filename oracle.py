import sys
import socket

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

if __name__ == "__main__":
    if len(sys.argv) != 2:
        print("Config file?\n")
        sys.exit(1)
        
    network_matrix = create_network_matrix(sys.argv[1])
    
    for row in network_matrix:
        print("\t".join(str(val) for val in row))

    server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    server_socket.bind(('0.0.0.0', 5000))
    server_socket.listen(26)

    num_nodes = len(network_matrix)
    node_map = {}
    client_sockets = {}
    seen_nodes = 0

    while (seen_nodes < num_nodes):
        client_socket, addr = server_socket.accept()
        data = client_socket.recv(6)
        if len(data) < 6:
            print("Received data is too short.")
            continue
        ip_addr = socket.inet_ntoa(data[:4])
        udp_port = int.from_bytes(data[4:6], byteorder='big')
        node_id = chr(ord('A') + seen_nodes)
        node_map[node_id] = (ip_addr, udp_port)
        client_sockets[node_id] = client_socket
        seen_nodes += 1
        
        print(f"Received connection from {ip_addr}:{udp_port}")

    static_message = b"Hello from Oracle!"
    for node_id, active_socket in client_sockets.items():
        active_socket.sendall(static_message)
        print(f"Successfully sent to Node {node_id}")
        

    