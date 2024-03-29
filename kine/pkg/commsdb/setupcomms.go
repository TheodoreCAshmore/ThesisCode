/**
 * @brief Package used to set up the gRPC connection with
 *
 */
package commsdb

import (
	"fmt"

	pb "github.com/k3s-io/kine/pkg/drivers/messagestructures"
	"google.golang.org/grpc"
	"google.golang.org/grpc/credentials/insecure"
)

var (
	Conn               *grpc.ClientConn
	Client             pb.DatabaseCommunicationClient
	ClientPublisher    pb.DatabaseCommunicationClient
	ClientQuery04      pb.DatabaseCommunicationClient
	ClientSmallQueries pb.DatabaseCommunicationClient
	Timestamp          string
)

func InitialiseCommsConnexion() {
	fmt.Println("# INITIALISING CONNECTION!")
	var err error
	Conn, err = grpc.Dial("0.0.0.0:50051", grpc.WithTransportCredentials(insecure.NewCredentials()))
	if err != nil {
		fmt.Println("# Bad business when creating the gRPC connection")
	}

	Client = pb.NewDatabaseCommunicationClient(Conn)
	ClientPublisher = pb.NewDatabaseCommunicationClient(Conn)
	ClientQuery04 = pb.NewDatabaseCommunicationClient(Conn)
	ClientSmallQueries = pb.NewDatabaseCommunicationClient(Conn)
}
