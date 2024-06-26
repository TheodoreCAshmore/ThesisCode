// Code generated by protoc-gen-go-grpc. DO NOT EDIT.
// versions:
// - protoc-gen-go-grpc v1.2.0
// - protoc             v4.23.1
// source: pkg/drivers/messagestructures/kinedata.proto

package messagestructures

import (
	context "context"
	grpc "google.golang.org/grpc"
	codes "google.golang.org/grpc/codes"
	status "google.golang.org/grpc/status"
)

// This is a compile-time assertion to ensure that this generated file
// is compatible with the grpc package it is being compiled against.
// Requires gRPC-Go v1.32.0 or later.
const _ = grpc.SupportPackageIsVersion7

// DatabaseCommunicationClient is the client API for DatabaseCommunication service.
//
// For semantics around ctx use and closing/ending streaming RPCs, please refer to https://pkg.go.dev/google.golang.org/grpc/?tab=doc#ClientConn.NewStream.
type DatabaseCommunicationClient interface {
	RequestPublish(ctx context.Context, in *KineInsert, opts ...grpc.CallOption) (*ResponsePublish, error)
	RequestDispose(ctx context.Context, in *KineDispose, opts ...grpc.CallOption) (*ResponseDispose, error)
	RequestMassDisposeCompaction(ctx context.Context, in *KineMassDisposeCompaction, opts ...grpc.CallOption) (*ResponseMassDisposeCompaction, error)
	RequestQueryGeneric(ctx context.Context, in *KineQueryGeneric, opts ...grpc.CallOption) (*ResponseQueryGeneric, error)
	RequestQueryThree(ctx context.Context, in *KineQueryThree, opts ...grpc.CallOption) (*ResponseQueryThree, error)
	RequestQueryCompactRevision(ctx context.Context, in *KineQueryCompactRevision, opts ...grpc.CallOption) (*ResponseQueryCompactRevision, error)
	RequestQueryMaximumId(ctx context.Context, in *KineQueryMaximumId, opts ...grpc.CallOption) (*ResponseQueryMaximumId, error)
	RequestTest(ctx context.Context, in *KineTest, opts ...grpc.CallOption) (*ResponseTest, error)
}

type databaseCommunicationClient struct {
	cc grpc.ClientConnInterface
}

func NewDatabaseCommunicationClient(cc grpc.ClientConnInterface) DatabaseCommunicationClient {
	return &databaseCommunicationClient{cc}
}

func (c *databaseCommunicationClient) RequestPublish(ctx context.Context, in *KineInsert, opts ...grpc.CallOption) (*ResponsePublish, error) {
	out := new(ResponsePublish)
	err := c.cc.Invoke(ctx, "/MessageStructures.DatabaseCommunication/RequestPublish", in, out, opts...)
	if err != nil {
		return nil, err
	}
	return out, nil
}

func (c *databaseCommunicationClient) RequestDispose(ctx context.Context, in *KineDispose, opts ...grpc.CallOption) (*ResponseDispose, error) {
	out := new(ResponseDispose)
	err := c.cc.Invoke(ctx, "/MessageStructures.DatabaseCommunication/RequestDispose", in, out, opts...)
	if err != nil {
		return nil, err
	}
	return out, nil
}

func (c *databaseCommunicationClient) RequestMassDisposeCompaction(ctx context.Context, in *KineMassDisposeCompaction, opts ...grpc.CallOption) (*ResponseMassDisposeCompaction, error) {
	out := new(ResponseMassDisposeCompaction)
	err := c.cc.Invoke(ctx, "/MessageStructures.DatabaseCommunication/RequestMassDisposeCompaction", in, out, opts...)
	if err != nil {
		return nil, err
	}
	return out, nil
}

func (c *databaseCommunicationClient) RequestQueryGeneric(ctx context.Context, in *KineQueryGeneric, opts ...grpc.CallOption) (*ResponseQueryGeneric, error) {
	out := new(ResponseQueryGeneric)
	err := c.cc.Invoke(ctx, "/MessageStructures.DatabaseCommunication/RequestQueryGeneric", in, out, opts...)
	if err != nil {
		return nil, err
	}
	return out, nil
}

func (c *databaseCommunicationClient) RequestQueryThree(ctx context.Context, in *KineQueryThree, opts ...grpc.CallOption) (*ResponseQueryThree, error) {
	out := new(ResponseQueryThree)
	err := c.cc.Invoke(ctx, "/MessageStructures.DatabaseCommunication/RequestQueryThree", in, out, opts...)
	if err != nil {
		return nil, err
	}
	return out, nil
}

func (c *databaseCommunicationClient) RequestQueryCompactRevision(ctx context.Context, in *KineQueryCompactRevision, opts ...grpc.CallOption) (*ResponseQueryCompactRevision, error) {
	out := new(ResponseQueryCompactRevision)
	err := c.cc.Invoke(ctx, "/MessageStructures.DatabaseCommunication/RequestQueryCompactRevision", in, out, opts...)
	if err != nil {
		return nil, err
	}
	return out, nil
}

func (c *databaseCommunicationClient) RequestQueryMaximumId(ctx context.Context, in *KineQueryMaximumId, opts ...grpc.CallOption) (*ResponseQueryMaximumId, error) {
	out := new(ResponseQueryMaximumId)
	err := c.cc.Invoke(ctx, "/MessageStructures.DatabaseCommunication/RequestQueryMaximumId", in, out, opts...)
	if err != nil {
		return nil, err
	}
	return out, nil
}

func (c *databaseCommunicationClient) RequestTest(ctx context.Context, in *KineTest, opts ...grpc.CallOption) (*ResponseTest, error) {
	out := new(ResponseTest)
	err := c.cc.Invoke(ctx, "/MessageStructures.DatabaseCommunication/RequestTest", in, out, opts...)
	if err != nil {
		return nil, err
	}
	return out, nil
}

// DatabaseCommunicationServer is the server API for DatabaseCommunication service.
// All implementations must embed UnimplementedDatabaseCommunicationServer
// for forward compatibility
type DatabaseCommunicationServer interface {
	RequestPublish(context.Context, *KineInsert) (*ResponsePublish, error)
	RequestDispose(context.Context, *KineDispose) (*ResponseDispose, error)
	RequestMassDisposeCompaction(context.Context, *KineMassDisposeCompaction) (*ResponseMassDisposeCompaction, error)
	RequestQueryGeneric(context.Context, *KineQueryGeneric) (*ResponseQueryGeneric, error)
	RequestQueryThree(context.Context, *KineQueryThree) (*ResponseQueryThree, error)
	RequestQueryCompactRevision(context.Context, *KineQueryCompactRevision) (*ResponseQueryCompactRevision, error)
	RequestQueryMaximumId(context.Context, *KineQueryMaximumId) (*ResponseQueryMaximumId, error)
	RequestTest(context.Context, *KineTest) (*ResponseTest, error)
	mustEmbedUnimplementedDatabaseCommunicationServer()
}

// UnimplementedDatabaseCommunicationServer must be embedded to have forward compatible implementations.
type UnimplementedDatabaseCommunicationServer struct {
}

func (UnimplementedDatabaseCommunicationServer) RequestPublish(context.Context, *KineInsert) (*ResponsePublish, error) {
	return nil, status.Errorf(codes.Unimplemented, "method RequestPublish not implemented")
}
func (UnimplementedDatabaseCommunicationServer) RequestDispose(context.Context, *KineDispose) (*ResponseDispose, error) {
	return nil, status.Errorf(codes.Unimplemented, "method RequestDispose not implemented")
}
func (UnimplementedDatabaseCommunicationServer) RequestMassDisposeCompaction(context.Context, *KineMassDisposeCompaction) (*ResponseMassDisposeCompaction, error) {
	return nil, status.Errorf(codes.Unimplemented, "method RequestMassDisposeCompaction not implemented")
}
func (UnimplementedDatabaseCommunicationServer) RequestQueryGeneric(context.Context, *KineQueryGeneric) (*ResponseQueryGeneric, error) {
	return nil, status.Errorf(codes.Unimplemented, "method RequestQueryGeneric not implemented")
}
func (UnimplementedDatabaseCommunicationServer) RequestQueryThree(context.Context, *KineQueryThree) (*ResponseQueryThree, error) {
	return nil, status.Errorf(codes.Unimplemented, "method RequestQueryThree not implemented")
}
func (UnimplementedDatabaseCommunicationServer) RequestQueryCompactRevision(context.Context, *KineQueryCompactRevision) (*ResponseQueryCompactRevision, error) {
	return nil, status.Errorf(codes.Unimplemented, "method RequestQueryCompactRevision not implemented")
}
func (UnimplementedDatabaseCommunicationServer) RequestQueryMaximumId(context.Context, *KineQueryMaximumId) (*ResponseQueryMaximumId, error) {
	return nil, status.Errorf(codes.Unimplemented, "method RequestQueryMaximumId not implemented")
}
func (UnimplementedDatabaseCommunicationServer) RequestTest(context.Context, *KineTest) (*ResponseTest, error) {
	return nil, status.Errorf(codes.Unimplemented, "method RequestTest not implemented")
}
func (UnimplementedDatabaseCommunicationServer) mustEmbedUnimplementedDatabaseCommunicationServer() {}

// UnsafeDatabaseCommunicationServer may be embedded to opt out of forward compatibility for this service.
// Use of this interface is not recommended, as added methods to DatabaseCommunicationServer will
// result in compilation errors.
type UnsafeDatabaseCommunicationServer interface {
	mustEmbedUnimplementedDatabaseCommunicationServer()
}

func RegisterDatabaseCommunicationServer(s grpc.ServiceRegistrar, srv DatabaseCommunicationServer) {
	s.RegisterService(&DatabaseCommunication_ServiceDesc, srv)
}

func _DatabaseCommunication_RequestPublish_Handler(srv interface{}, ctx context.Context, dec func(interface{}) error, interceptor grpc.UnaryServerInterceptor) (interface{}, error) {
	in := new(KineInsert)
	if err := dec(in); err != nil {
		return nil, err
	}
	if interceptor == nil {
		return srv.(DatabaseCommunicationServer).RequestPublish(ctx, in)
	}
	info := &grpc.UnaryServerInfo{
		Server:     srv,
		FullMethod: "/MessageStructures.DatabaseCommunication/RequestPublish",
	}
	handler := func(ctx context.Context, req interface{}) (interface{}, error) {
		return srv.(DatabaseCommunicationServer).RequestPublish(ctx, req.(*KineInsert))
	}
	return interceptor(ctx, in, info, handler)
}

func _DatabaseCommunication_RequestDispose_Handler(srv interface{}, ctx context.Context, dec func(interface{}) error, interceptor grpc.UnaryServerInterceptor) (interface{}, error) {
	in := new(KineDispose)
	if err := dec(in); err != nil {
		return nil, err
	}
	if interceptor == nil {
		return srv.(DatabaseCommunicationServer).RequestDispose(ctx, in)
	}
	info := &grpc.UnaryServerInfo{
		Server:     srv,
		FullMethod: "/MessageStructures.DatabaseCommunication/RequestDispose",
	}
	handler := func(ctx context.Context, req interface{}) (interface{}, error) {
		return srv.(DatabaseCommunicationServer).RequestDispose(ctx, req.(*KineDispose))
	}
	return interceptor(ctx, in, info, handler)
}

func _DatabaseCommunication_RequestMassDisposeCompaction_Handler(srv interface{}, ctx context.Context, dec func(interface{}) error, interceptor grpc.UnaryServerInterceptor) (interface{}, error) {
	in := new(KineMassDisposeCompaction)
	if err := dec(in); err != nil {
		return nil, err
	}
	if interceptor == nil {
		return srv.(DatabaseCommunicationServer).RequestMassDisposeCompaction(ctx, in)
	}
	info := &grpc.UnaryServerInfo{
		Server:     srv,
		FullMethod: "/MessageStructures.DatabaseCommunication/RequestMassDisposeCompaction",
	}
	handler := func(ctx context.Context, req interface{}) (interface{}, error) {
		return srv.(DatabaseCommunicationServer).RequestMassDisposeCompaction(ctx, req.(*KineMassDisposeCompaction))
	}
	return interceptor(ctx, in, info, handler)
}

func _DatabaseCommunication_RequestQueryGeneric_Handler(srv interface{}, ctx context.Context, dec func(interface{}) error, interceptor grpc.UnaryServerInterceptor) (interface{}, error) {
	in := new(KineQueryGeneric)
	if err := dec(in); err != nil {
		return nil, err
	}
	if interceptor == nil {
		return srv.(DatabaseCommunicationServer).RequestQueryGeneric(ctx, in)
	}
	info := &grpc.UnaryServerInfo{
		Server:     srv,
		FullMethod: "/MessageStructures.DatabaseCommunication/RequestQueryGeneric",
	}
	handler := func(ctx context.Context, req interface{}) (interface{}, error) {
		return srv.(DatabaseCommunicationServer).RequestQueryGeneric(ctx, req.(*KineQueryGeneric))
	}
	return interceptor(ctx, in, info, handler)
}

func _DatabaseCommunication_RequestQueryThree_Handler(srv interface{}, ctx context.Context, dec func(interface{}) error, interceptor grpc.UnaryServerInterceptor) (interface{}, error) {
	in := new(KineQueryThree)
	if err := dec(in); err != nil {
		return nil, err
	}
	if interceptor == nil {
		return srv.(DatabaseCommunicationServer).RequestQueryThree(ctx, in)
	}
	info := &grpc.UnaryServerInfo{
		Server:     srv,
		FullMethod: "/MessageStructures.DatabaseCommunication/RequestQueryThree",
	}
	handler := func(ctx context.Context, req interface{}) (interface{}, error) {
		return srv.(DatabaseCommunicationServer).RequestQueryThree(ctx, req.(*KineQueryThree))
	}
	return interceptor(ctx, in, info, handler)
}

func _DatabaseCommunication_RequestQueryCompactRevision_Handler(srv interface{}, ctx context.Context, dec func(interface{}) error, interceptor grpc.UnaryServerInterceptor) (interface{}, error) {
	in := new(KineQueryCompactRevision)
	if err := dec(in); err != nil {
		return nil, err
	}
	if interceptor == nil {
		return srv.(DatabaseCommunicationServer).RequestQueryCompactRevision(ctx, in)
	}
	info := &grpc.UnaryServerInfo{
		Server:     srv,
		FullMethod: "/MessageStructures.DatabaseCommunication/RequestQueryCompactRevision",
	}
	handler := func(ctx context.Context, req interface{}) (interface{}, error) {
		return srv.(DatabaseCommunicationServer).RequestQueryCompactRevision(ctx, req.(*KineQueryCompactRevision))
	}
	return interceptor(ctx, in, info, handler)
}

func _DatabaseCommunication_RequestQueryMaximumId_Handler(srv interface{}, ctx context.Context, dec func(interface{}) error, interceptor grpc.UnaryServerInterceptor) (interface{}, error) {
	in := new(KineQueryMaximumId)
	if err := dec(in); err != nil {
		return nil, err
	}
	if interceptor == nil {
		return srv.(DatabaseCommunicationServer).RequestQueryMaximumId(ctx, in)
	}
	info := &grpc.UnaryServerInfo{
		Server:     srv,
		FullMethod: "/MessageStructures.DatabaseCommunication/RequestQueryMaximumId",
	}
	handler := func(ctx context.Context, req interface{}) (interface{}, error) {
		return srv.(DatabaseCommunicationServer).RequestQueryMaximumId(ctx, req.(*KineQueryMaximumId))
	}
	return interceptor(ctx, in, info, handler)
}

func _DatabaseCommunication_RequestTest_Handler(srv interface{}, ctx context.Context, dec func(interface{}) error, interceptor grpc.UnaryServerInterceptor) (interface{}, error) {
	in := new(KineTest)
	if err := dec(in); err != nil {
		return nil, err
	}
	if interceptor == nil {
		return srv.(DatabaseCommunicationServer).RequestTest(ctx, in)
	}
	info := &grpc.UnaryServerInfo{
		Server:     srv,
		FullMethod: "/MessageStructures.DatabaseCommunication/RequestTest",
	}
	handler := func(ctx context.Context, req interface{}) (interface{}, error) {
		return srv.(DatabaseCommunicationServer).RequestTest(ctx, req.(*KineTest))
	}
	return interceptor(ctx, in, info, handler)
}

// DatabaseCommunication_ServiceDesc is the grpc.ServiceDesc for DatabaseCommunication service.
// It's only intended for direct use with grpc.RegisterService,
// and not to be introspected or modified (even as a copy)
var DatabaseCommunication_ServiceDesc = grpc.ServiceDesc{
	ServiceName: "MessageStructures.DatabaseCommunication",
	HandlerType: (*DatabaseCommunicationServer)(nil),
	Methods: []grpc.MethodDesc{
		{
			MethodName: "RequestPublish",
			Handler:    _DatabaseCommunication_RequestPublish_Handler,
		},
		{
			MethodName: "RequestDispose",
			Handler:    _DatabaseCommunication_RequestDispose_Handler,
		},
		{
			MethodName: "RequestMassDisposeCompaction",
			Handler:    _DatabaseCommunication_RequestMassDisposeCompaction_Handler,
		},
		{
			MethodName: "RequestQueryGeneric",
			Handler:    _DatabaseCommunication_RequestQueryGeneric_Handler,
		},
		{
			MethodName: "RequestQueryThree",
			Handler:    _DatabaseCommunication_RequestQueryThree_Handler,
		},
		{
			MethodName: "RequestQueryCompactRevision",
			Handler:    _DatabaseCommunication_RequestQueryCompactRevision_Handler,
		},
		{
			MethodName: "RequestQueryMaximumId",
			Handler:    _DatabaseCommunication_RequestQueryMaximumId_Handler,
		},
		{
			MethodName: "RequestTest",
			Handler:    _DatabaseCommunication_RequestTest_Handler,
		},
	},
	Streams:  []grpc.StreamDesc{},
	Metadata: "pkg/drivers/messagestructures/kinedata.proto",
}
