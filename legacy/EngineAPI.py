import win32file, pywintypes, win32console

import logger
import subprocess
import random
import string
import time
import atexit
import struct

import ctypes
from typing import List
from Move import * 

ENGINE_DLL_PATH = ""
ENGINE_DLL: ctypes.CDLL = None
_engine_create_instance = None
_engine_exists_instance = None
_engine_destroy_instance = None
_engine_request_command = None

START = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"

COMMAND_STR_B = ["NONE", "SET", "MOVE", "UNDO", "MOVEREQ", "GENMOVES", "CHECK", "CHECKMATE", "FEN", "COLOR", "EXIT"]
COLOR_STR_B = ["WHITE", "BLACK"]

BUF_LEN = 1024
OK = bytearray(b'\xFF\xFF')

BLACK = 1
WHITE = 0

# List of available commands
NONE      = 0x00    # Payload: None
SET       = 0x01    # Payload: Fen string ([FEN_STR_LEN] BYTES)
MOVE      = 0x02    # Payload: Moving player (1 BYTE) + Move (2 BYTES)
UNDO      = 0x03    # Payload: None
MOVEREQ   = 0x04    # Payload: AI Level (1 BYTE) + AI Color (1 BYTE)
GENMOVES  = 0x05    # Payload: Square (1 BYTE) of the piece to generate moves of (100 + [COLOR] to generate all the legal move for a specific color)
CHECK     = 0x06    # Payload: Color (1 BYTE)
CHECKMATE = 0x07    # Payload: Color (1 BYTE)
FEN       = 0x08    # Payload: None
COLOR     = 0x09    # Payload: None
EXIT      = 0x0A    # Payload: None

# Structure of the command

#       HEADER            ------- Payload ([PAYLEN] BYTES)
#  ----------------      |
# |                |     v
# 0x00 | 0x00 0x00 | 0x00 ...
#   ^       ^
#   |       | 
#   |        --------------- Payload length (2 BYTES)
#    -------- Command identifier (1 BYTE)

def initialize_engine_dll(path: str):
    global ENGINE_DLL_PATH
    global ENGINE_DLL 
    
    global _engine_create_instance
    global _engine_exists_instance
    global _engine_destroy_instance 
    global _engine_request_command


    ENGINE_DLL_PATH = path
    ENGINE_DLL = ctypes.CDLL(path)

    _engine_create_instance     =   ENGINE_DLL.CreateInstance
    _engine_exists_instance     =   ENGINE_DLL.ExistsInstance
    _engine_destroy_instance    =   ENGINE_DLL.DestroyInstance
    _engine_request_command     =   ENGINE_DLL.RequestCommand

    # Creating global functors pointing to dll's functions
    _engine_create_instance.argtypes = [ctypes.c_uint16, ctypes.c_bool]
    _engine_create_instance.restype = ctypes.c_bool


    _engine_exists_instance.argtypes = [ctypes.c_uint16]
    _engine_exists_instance.restype = ctypes.c_bool


    _engine_destroy_instance.argtypes = [ctypes.c_uint16]
    _engine_destroy_instance.restype = ctypes.c_bool

    _engine_request_command.argtypes = [ctypes.c_uint16, Command]
    _engine_request_command.restype = Response

# DLL's structs
    
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



class Engine():
    def __init__(self, engine_id: ctypes.c_uint16 = None, debug_console: bool = False):
        if (ENGINE_DLL == None):
            raise Exception("Engine dll not initialized! Initialize the Engine's dll using EngineAPI.initialize_engine_dll(path_to_dll)")

        if (engine_id != None and _engine_exists_instance(engine_id)): 
            self.__engine_id = engine_id
        else:
            self.__engine_id = self.generate_unique_id()

        _engine_create_instance(self.__engine_id, debug_console)
        
    #     atexit.register(self.cleanup)

    # def cleanup(self):
    #     if (self.engine_handle != None):
    #         self.exit()
        
    def generate_unique_id(self) -> int:
        return random.randrange(0, 0xFFFF)
    
    def set(self, fen_str: str):
        rsp = self.send_command(SET, fen_str.encode())
        
        if (rsp != OK):
            logger.error("Could not set the engine at given position!")

    def move(self, playing_color: int, move: Move):
        rsp = self.send_command(MOVE, bytearray(playing_color.to_bytes(1) + move.getInternal().to_bytes(2, "little")))

        if (rsp != OK):
            logger.error("Could not play the move!")
        
    def undo(self):
        rsp = self.send_command(UNDO)

        if (rsp != OK):
            logger.error("Could not undo the last move!")

    def ai_move(self, ai_level: int, ai_color: int) -> Move:
        move_internal = self.send_command(MOVEREQ, bytearray([ai_level, ai_color]))

        return Move(int.from_bytes(move_internal, "little"))
    
    def generate_moves(self, square: int) -> List[Move]:
        move_array = self.send_command(GENMOVES, square.to_bytes(1))

        return arrayU16toMoves(move_array)
    
    def in_check(self, color: int) -> bool:
        result = self.send_command(CHECK, color.to_bytes(1))

        return bool(int.from_bytes(result, "little"))
    
    def fen(self) -> str:
        fen_str = self.send_command(FEN)

        return fen_str.decode()
    
    def checkmate(self, color: int):
        result = self.send_command(CHECKMATE, color.to_bytes(1))

        return bool(int.from_bytes(result, "little"))
    
    def color(self):
        result = self.send_command(COLOR)

        return int.from_bytes(result, "little")
    
    def exit(self):
        done = _engine_destroy_instance(self.__engine_id)

        if (not done):
            logger.error("Could exit the engine!")

        self.__engine_id = None
            
    def send_command(self, command_id: CommandType, payload: bytearray = bytearray()) -> bytearray:
        if command_id > EXIT or command_id < NONE: 
            raise Exception("Invalid command type!")
        
        ctypes_payload = (ctypes.c_uint8 * 256)(*payload)
        cmd = Command(header=CommandHeader(type=command_id, payload_size=len(payload)), payload=ctypes_payload)

        resp: Response = _engine_request_command(self.__engine_id, cmd)

        return bytearray(resp.payload)[:resp.payload_size] 
        # try:
        #     # According to the payload header specification
        #     command_header = struct.pack("<HH", command_id, len(payload))

        #     # for b in command_header: print(b)

        #     _, _ = win32file.WriteFile(self.engine_handle, command_header + payload)
        #     _, rsp = win32file.ReadFile(self.engine_handle, 1)

        #     if int.from_bytes(rsp, "little") != ACK:
        #         self.send_command(command_id, payload, attempt + 1) 
        # except pywintypes.error as e:
        #     logger.error(f"Could not send {COMMAND_STR_B[command_id]} command! ERROR CODE: {e.args[0]}")
        #     return -1


    # def recv_response(self) -> bytearray: 
    #     try:
    #         _, rsp = win32file.ReadFile(self.engine_handle, BUF_LEN)

    #         _, _ = win32file.WriteFile(self.engine_handle, ACK.to_bytes(1))

    #         return rsp
    #     except pywintypes.error as e:
    #         logger.error(f"Could not receive response! ERROR CODE: {e.args[0]}")
        