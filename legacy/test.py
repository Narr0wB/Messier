import ctypes
import os
from Move import *

# ctypes.windll.AddDllDirectory(os.getcwd() + "/build")

START = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"

engine_dll = ctypes.CDLL("build/Engine.dll")

class CommandType(ctypes.c_uint16):
    NONE, SET, MOVE, UNDO, MOVEREQ, GENMOVES, CHECK, CHECKMATE, FEN, COLOR = range(10)

class CommandHeader(ctypes.Structure):
    _fields_ = [
        ("type", CommandType),
        ("payload_size", ctypes.c_uint16)
    ]

class Command(ctypes.Structure):
    _fields_ = [
        ("header", CommandHeader),
        ("payload", ctypes.c_uint8 * 256)
    ]

class Response(ctypes.Structure):
    _fields_ = [
        ("payload_size", ctypes.c_uint16),
        ("payload", ctypes.c_uint8 * 512)
    ]

engine_create_instance = engine_dll.CreateInstance

engine_create_instance.argtypes = [ctypes.c_uint16, ctypes.c_bool]
engine_create_instance.restype = ctypes.c_bool

print(engine_create_instance(1000, False))
print(engine_create_instance(2002, True))

engine_request_command = engine_dll.RequestCommand

engine_request_command.argtypes = [ctypes.c_uint16, Command]
engine_request_command.restype = Response

resp = engine_request_command(2002, Command(header=CommandHeader(type=CommandType.SET, payload_size=len(START)), payload= (ctypes.c_ubyte * 256)(*map(ord, START)) ))
print("SET COMMAND RESPONSE", hex(resp.payload[0]), hex(resp.payload[1]))


move = Move(from_sq=10, to_sq=18, flags=QUIET)
cmd = Command(header=CommandHeader(type=CommandType.MOVE, payload_size=2))
ctypes.memmove(cmd.payload, move.getInternal().to_bytes(2, "little"), 2)

cmd2 = Command(header=CommandHeader(type=CommandType.GENMOVES, payload_size=1), payload=(ctypes.c_uint8(10), ))


resp = engine_request_command(1000, cmd)
                              
second_resp = engine_request_command(2002, cmd2)

print(Move(second_resp.payload[1] << 8 | second_resp.payload[0]))
print(hex(resp.payload[0]), hex(resp.payload[1]))