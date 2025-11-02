import subprocess
import threading
import time
import random
import tkinter as tk
import time
import os
from math import *
import atexit

from Event import *
from Utilities import *
from EngineAPI import * 
from Move import *
from pygame import mixer

import win32file, pywintypes
from PIL import Image, ImageTk
from PIL.Image import Resampling

from ctypes import windll
windll.shcore.SetProcessDpiAwareness(1)
mixer.init()
dir_path = os.path.dirname(os.path.realpath(__file__))

CHESS_START = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"
initialize_engine_dll("build/Engine.dll")

# BUGGED POSITION: 2kr1bnr/ppp1pppp/4b3/8/1nP2B2/N7/PPK1PPPP/R4BNR w - - 0 1

# -------------------------------------------------------------------------------
KING = 0
QUEEN = 1
BISHOP = 2
KNIGHT = 3
ROOK = 4
PAWN = 5

WHITE = 0
BLACK = 1

# -------------------------------------------------------------------------------
# NONE = 0x00
# SET = 0x01
# MOVE = 0x02
# UNDO = 0x03
# MOVEREQ = 0x04
# GENMOVES = 0x05
# INCHECK = 0x06
# FEN = 0x07
# EXIT = 0x08

# -------------------------------------------------------------------------------
SQSTR = {
	"a1": 0, "b1": 1, "c1": 2, "d1": 3, "e1": 4, "f1": 5, "g1": 6, "h1": 7,
	"a2": 8, "b2": 9, "c2": 10, "d2": 11, "e2": 12, "f2": 13, "g2": 14, "h2": 15,
	"a3": 16, "b3": 17, "c3": 18, "d3": 19, "e3": 20, "f3": 21, "g3": 22, "h3": 23,
	"a4": 24, "b4": 25, "c4": 26, "d4": 27, "e4": 28, "f4": 29, "g4": 30, "h4": 31,
	"a5": 32, "b5": 33, "c5": 34, "d5": 35, "e5": 36, "f5": 37, "g5": 38, "h5": 39,
	"a6": 40, "b6": 41, "c6": 42, "d6": 43, "e6": 44, "f6": 45, "g6": 46, "h6": 47,
	"a7": 48, "b7": 49, "c7": 50, "d7": 51, "e7": 52, "f7": 53, "g7": 54, "h7": 55,
	"a8": 56, "b8": 57, "c8": 58, "d8": 59, "e8": 60, "f8": 61, "g8": 62, "h8": 63,
	"None": 64
}

# MOVE_TYPESTR_B = [
# 	"", "", " O-O", " O-O-O", " N (promotion)", " B (promotion)", " R (promotion)", " Q (promotion)", " (capture)", "", " e.p.", "",
# 	" N (promotion)", " B (promotion)", " R (promotion)", " Q (promotion)"
# ]

# MOVE_FLAGSTR = [
#     "", "", "OO", "OOO", "", "", "", "", "x", "", "ep", "", "", "", "", ""
# ]

# -------------------------------------------------------------------------------
# QUIET = 0b0000
# DOUBLE_PUSH = 0b0001
# OO = 0b0010
# OOO = 0b0011
# CAPTURE = 0b1000
# CAPTURES = 0b1111
# EN_PASSANT = 0b1010
# PROMOTIONS = 0b0111
# PROMOTION_CAPTURES = 0b1100
# PR_KNIGHT = 0b0100
# PR_BISHOP = 0b0101
# PR_ROOK = 0b0110
# PR_QUEEN = 0b0111
# PC_KNIGHT = 0b1100
# PC_BISHOP = 0b1101
# PC_ROOK = 0b1110
# PC_QUEEN = 0b1111

NOMOVE = 0xFF

# -------------------------------------------------------------------------------

pieceToFen = {
    (KING, BLACK) : "k",
    (QUEEN, BLACK) : "q",
    (BISHOP, BLACK) : "b",
    (KNIGHT, BLACK) : "n",
    (ROOK, BLACK) : "r",
    (PAWN, BLACK) : "p",
    (KING, WHITE) : "K",
    (QUEEN, WHITE) : "Q",
    (BISHOP, WHITE) : "B",
    (KNIGHT, WHITE) : "N",
    (ROOK, WHITE) : "R",
    (PAWN, WHITE) : "P",
    }

fenToPiece = {
    "k" : (KING, BLACK),
    "q" : (QUEEN, BLACK),
    "b" : (BISHOP, BLACK),
    "n" : (KNIGHT, BLACK),
    "r" : (ROOK, BLACK),
    "p" : (PAWN, BLACK),
    "K" : (KING, WHITE),
    "Q" : (QUEEN, WHITE),
    "B" : (BISHOP, WHITE),
    "N" : (KNIGHT, WHITE),
    "R" : (ROOK, WHITE),
    "P" : (PAWN, WHITE),
    }

columnToLetter = {
    0 : "a",
    1 : "b",
    2 : "c",
    3 : "d",
    4 : "e",
    5 : "f",
    6 : "g",
    7 : "h"
}

letterToColumn = {
    "a" : 0,
    "b" : 1,
    "c" : 2,
    "d" : 3,
    "e" : 4,
    "f" : 5,
    "g" : 6,
    "h" : 7
}

def flagToType(flag: int) -> int:
    if (flag < PR_KNIGHT or flag > PC_QUEEN):
        return -1
    
    if flag == PR_KNIGHT or flag == PC_KNIGHT:
        return KNIGHT
    if flag == PR_BISHOP or flag == PC_BISHOP:
        return BISHOP
    if flag == PR_ROOK or flag == PC_ROOK:
        return ROOK
    if flag == PR_QUEEN or flag == PC_QUEEN:
        return QUEEN
    
def typeToFlag(type: int, capture: bool) -> int:
    if type == KNIGHT:
        if capture:
            return PC_KNIGHT
        else:
            return PR_KNIGHT
    
    if type == BISHOP:
        if capture:
            return PC_BISHOP
        else:
            return PR_BISHOP
        
    if type == ROOK:
        if capture:
            return PC_ROOK
        else:
            return PR_ROOK
        
    if type == QUEEN:
        if capture:
            return PC_QUEEN
        else:
            return PR_QUEEN
        
    return -1

class Piece:
    def __init__(self, pieceType: int, color: int, image: ImageTk.PhotoImage):
        self.type = pieceType
        self.color = color
        self.image = image
        self.fentype = pieceToFen[(pieceType, color)]
        self.uniqueCode = 0

class Board():
    def __init__(self, debug=False, tileSize=105, colorLight="#F1D9C2", colorDark="#AB7966", start: str = CHESS_START, ai: bool = False):
        self.debug = debug

        self.parent = tk.Tk()
        self.should_close = False
        self.rows = 8
        self.columns = 8
        self.fullscreen = True
        self.size = tileSize
        self.piece_scale = 0.82
        self.board_color_light = colorLight
        self.board_color_dark = colorDark
        self.selected_piece_color = "#DED080"
        self.move_color = "#D0AC6A"
        self.application_fps = 240

        self.fenToImage = {
        "k" : ImageTk.PhotoImage(Image.open(f"{dir_path}\\assets\\bking.png").resize((int(self.piece_scale * self.size),int(self.piece_scale * self.size)), Resampling.LANCZOS)),
        "q" : ImageTk.PhotoImage(Image.open(f"{dir_path}\\assets\\bqueen.png").resize((int(self.piece_scale * self.size),int(self.piece_scale * self.size)), Resampling.LANCZOS)),
        "n" : ImageTk.PhotoImage(Image.open(f"{dir_path}\\assets\\bhorse.png").resize((int(self.piece_scale * self.size),int(self.piece_scale * self.size)), Resampling.LANCZOS)),
        "r" : ImageTk.PhotoImage(Image.open(f"{dir_path}\\assets\\brook.png").resize((int(self.piece_scale * self.size),int(self.piece_scale * self.size)), Resampling.LANCZOS)),
        "b" : ImageTk.PhotoImage(Image.open(f"{dir_path}\\assets\\bbishop.png").resize((int(self.piece_scale * self.size),int(self.piece_scale * self.size)), Resampling.LANCZOS)),
        "p" : ImageTk.PhotoImage(Image.open(f"{dir_path}\\assets\\bpawn.png").resize((int(self.piece_scale * self.size),int(self.piece_scale * self.size)), Resampling.LANCZOS)),
        "K" : ImageTk.PhotoImage(Image.open(f"{dir_path}\\assets\\wking.png").resize((int(self.piece_scale * self.size),int(self.piece_scale * self.size)), Resampling.LANCZOS)),
        "Q" : ImageTk.PhotoImage(Image.open(f"{dir_path}\\assets\\wqueen.png").resize((int(self.piece_scale * self.size),int(self.piece_scale * self.size)), Resampling.LANCZOS)),
        "N" : ImageTk.PhotoImage(Image.open(f"{dir_path}\\assets\\whorse.png").resize((int(self.piece_scale * self.size),int(self.piece_scale * self.size)), Resampling.LANCZOS)),
        "R" : ImageTk.PhotoImage(Image.open(f"{dir_path}\\assets\\wrook.png").resize((int(self.piece_scale * self.size),int(self.piece_scale * self.size)), Resampling.LANCZOS)),
        "B" : ImageTk.PhotoImage(Image.open(f"{dir_path}\\assets\\wbishop.png").resize((int(self.piece_scale * self.size),int(self.piece_scale * self.size)), Resampling.LANCZOS)),
        "P" : ImageTk.PhotoImage(Image.open(f"{dir_path}\\assets\\wpawn.png").resize((int(self.piece_scale * self.size),int(self.piece_scale * self.size)), Resampling.LANCZOS)),
        }

        self.selected_piece = ()
        self.moves = []
        self.use_ai = ai
        self.ai_level = 10
        self.game_ply = 0
        self.start_player = WHITE
        self.engine = Engine(debug_console=debug)

        self.board = {}
        self.move_log = []

        self.promotion_ui = False
        self.animations_done = True

        canvas_width = self.columns * self.size
        canvas_height = self.rows * self.size

        self.parent.geometry(f"{(self.size+30)*8}x{(self.size+30)*8}")
        self.parent.attributes("-fullscreen", self.fullscreen)

        self.canvas = tk.Canvas(self.parent, borderwidth=0, highlightthickness=0,
                                width=canvas_width+1, height=canvas_height+1, background="#302E2B")
        self.canvas.pack(fill="both", expand=True)

        self.parent.update()
        self.promotion_type = None
        self.window_width = self.parent.winfo_width()
        self.window_height = self.parent.winfo_height()
        self.board_offset_x = ((self.window_width - (self.size*8)) // 3) * 2
        self.board_offset_y = ((self.window_height - (self.size*8)) // 2) * 1

        self.canvas.bind("<Button-1>", self.onClick)
        self.canvas.bind("<B1-Motion>", self.onDrag)
        self.canvas.bind("<ButtonRelease-1>", self.onRelease)
        self.parent.bind("<F5>", lambda e: self.loadBoard(start))
        self.parent.bind("<F6>", self.restoreMove)
        self.parent.bind("<F11>", self.toggleFullScreen)
        self.canvas.bind("<<restore>>", self._restoreMove)

        self.parent.protocol("WM_DELETE_WINDOW", self.onClose)
        if self.debug:
            self.parent.bind("<F8>", lambda e: print(self.toFen()))

        # DEBUG
        def debug_atexit(board):
            try:
                print(board.toFen())
                board.engineSendCommand(EXIT)
            except Exception as e:
                pass

        atexit.register(debug_atexit, self)

        self.handler = Handler()
        self.handler.addHandle(ENGINE_MOVE_EVENT, self.moveAI)
        self.handler.addHandle(UPDATE_LOG_EVENT, self.updateMoveLog)

        self.gui_font = "Noto Sans Medium"
        self.gui = {
            "offx": 0, 
            "offy": 0, 
            "width": 0, 
            "height": 0
        }

        self._control_variables = {
            "engine_calculating": False, 
            "movelog_count": 2, 
            "movelog_list": [], 
            "drag_calls": 0, 
            "restore": False, 
            "promotion_sq": None
        }
        
        
        self.loading_animation = []
        loading_gif = Image.open(f"{dir_path}\\assets\\anim.gif")

        for i in range(4):
            loading_gif.seek(i)
            self.loading_animation.append(ImageTk.PhotoImage(loading_gif.resize((18, 18), resample=Resampling.NEAREST)))

        self.loadBoard(start)
    
    def toggleFullScreen(self, event):
        self.fullscreen = not self.fullscreen
        self.parent.attributes("-fullscreen", self.fullscreen)

    def onResize(self, event):

        pass

    def onClose(self):
        try:
            print(self.toFen())
        except:
            pass

        self.engine.exit()

        self.should_close = True

    def mainloop(self):
        while not self.should_close:
            time.sleep(1/self.application_fps)
            self.handler.pollEvents()            

            self.parent.update()
            self.parent.update_idletasks()
    
    def updateMoveLog(self, event: UpdateLogEvent):
        top_offset = self.gui["offy"] +int(self.gui["height"] * 0.15)
        side_offset = self.gui["offx"] + int(self.gui["width"] * 0.06)
        element_offset = int(self.gui["height"] * 0.05)

        animation_frames = 100

        if event.restore:
            gui_move_element: GuiMoveElement = self._control_variables["movelog_list"][-1]
            self._control_variables["movelog_count"] -= (-1 if self._control_variables["movelog_count"] == 1 else 1)

            if (self.current_player != self.start_player):
                gui_move_element.updateBlack("") if self.start_player == WHITE else gui_move_element.updateWhite("")
            else:
                self._control_variables["movelog_list"].pop(-1)
                self.canvas.delete(gui_move_element.getId())
                self._moveItem(f"move_log", 0, -element_offset, animation_frames)

                if (len(self._control_variables["movelog_list"]) > 9):
                    self._control_variables["movelog_list"][-10].draw(side_offset, top_offset + 9*element_offset)

            if self._control_variables["restore"]:
                self._control_variables["restore"] = False
                self.canvas.event_generate("<<restore>>", when="tail")
            else:
                self.parent.bind("<F6>", self.restoreMove)

            return
        
        if (self._control_variables["movelog_count"] == 2):
            self._control_variables["movelog_count"] = 0

            if (len(self._control_variables["movelog_list"]) > 9):
                self.canvas.delete(self._control_variables["movelog_list"][-10].getId())

            self._moveItem(f"move_log", 0, element_offset, animation_frames)

            self._control_variables["movelog_list"].append(GuiMoveElement(self.canvas, side_offset, top_offset, int(self.gui["width"] * 0.35), int(self.gui["height"] * 0.04), int((event.ply + 1)/2), "", "", fill="#21201E"))   
        self._control_variables["movelog_count"] += 1

        if (self.current_player == WHITE):
            self._control_variables["movelog_list"][-1].updateBlack(((event.fen_type if (event.move.getFlags() != OOO and event.move.getFlags() != OO) else "") + MOVE_FLAGSTR[event.move.getFlags()]) + event.move.__str__()[2:4])
        else:
            self._control_variables["movelog_list"][-1].updateWhite(((event.fen_type if (event.move.getFlags() != OOO and event.move.getFlags() != OO) else "") + MOVE_FLAGSTR[event.move.getFlags()]) + event.move.__str__()[2:4])

        

        #print(f"it took {end-start}s")

    # Add a piece to self.board and assign each piece a unique code to be able to distinguish between same type&color pieces
    def createPiece(self, square: int, piece_type: int, piece_color: int): 
        piece = Piece(piece_type, piece_color, self.fenToImage[pieceToFen[piece_type, piece_color]])

        uC = random.randint(0x00FF, 0xFFFF)
        
        same = True
        while (same):
            same = False
            for item in self.board.values():
                if (item.uniqueCode == f"pc{uC}"):
                    uC = random.randint(0x00FF, 0xFFFF)
                    same = True

        piece.uniqueCode = f"pc{uC}"
        self.board[square] = piece

    def requestAIMove(self):
        self.canvas.unbind("<Button-1>")
        self.canvas.unbind("<B1-Motion>")

        #self.engineSendCommand(MOVEREQ, bytearray([self.ai_level, self.current_player]), wait = False)

        engine_move_thread = threading.Thread(target=self.moveAIThread, args=())
        engine_move_thread.start()

        self._control_variables["engine_calculating"] = True

        self.canvas.itemconfig("engine_text", text="Calculating...")
        self.playCalculatingAnimation(0)

    def playCalculatingAnimation(self, idx):
        self.canvas.delete("anim")

        self.canvas.create_image(self.gui["offx"] + int(self.gui["width"] * 0.05), self.gui["offy"] + int(self.gui["height"] * 0.056), image=self.loading_animation[idx], tags=("anim",), anchor=tk.W)
        
        idx += 1
        idx %= len(self.loading_animation)
        
        if not self._control_variables["engine_calculating"]:
            self.canvas.delete("anim")
            self.canvas.itemconfig("engine_text", text="")
            return
        
        self.canvas.after(140, self.playCalculatingAnimation, idx)

    def moveAIThread(self):
        #res = win32file.ReadFile(engine_pipe_handle, 1024)
        res = self.engine.ai_move(self.ai_level, self.current_player)
        print(res)

        self.handler.queuePush(EngineMoveEvent(res))

    def moveAI(self, event: EngineMoveEvent):
        self._control_variables["engine_calculating"] = False

        if self.debug:
            print("The engine played", event.move)

        self.playMove(event.move)

        self.canvas.bind("<Button-1>", self.onClick)
        self.canvas.bind("<B1-Motion>", self.onDrag)

    def restoreMove(self, event):
        self.parent.unbind("<F6>")
        self.canvas.event_generate("<<restore>>", when="tail")

        if self.use_ai:
            self._control_variables["restore"] = True

    def _restoreMove(self, event):
        if len(self.move_log) == 0 or self.promotion_ui or self._control_variables["engine_calculating"]:
            self.parent.bind("<F6>", self.restoreMove)
            return
        
        if (self.selected_piece != None):
            self.resetSelectedMovesColor([move.getTo() for move in self.moves])
            self.resetSelectedColor()

        self.engine.undo()
        self.current_player ^= 1
        
        last_move, last_move_capture = self.move_log.pop(-1)

        from_sq = last_move.getFrom()
        to_sq = last_move.getTo()
        flags = last_move.getFlags()

        self.movePiece(to_sq, from_sq)

        if flags == EN_PASSANT:
            if self.current_player == BLACK:
                self.board[to_sq + 8] = last_move_capture
                self.drawPiece(last_move_capture, to_sq + 8)
            else:
                self.board[to_sq - 8] = last_move_capture
                self.drawPiece(last_move_capture, to_sq - 8)
        
        if flags == CAPTURE or flags >= PC_KNIGHT:
            self.board[to_sq] = last_move_capture
            self.drawPiece(last_move_capture, to_sq)
        
        if flags >= PR_KNIGHT and flags != CAPTURE:
            self.canvas.delete(self.board[from_sq].uniqueCode)
            self.createPiece(from_sq, PAWN, self.current_player)
            self.drawPiece(self.board[from_sq], from_sq)

        if flags == OO:
            if self.current_player == BLACK:
                self.movePiece(SQSTR["f8"], SQSTR["h8"])
            else:
                self.movePiece(SQSTR["f1"], SQSTR["h1"])

        if flags == OOO:
            if self.current_player == BLACK:
                self.movePiece(SQSTR["d8"], SQSTR["a8"])
            else:
                self.movePiece(SQSTR["d1"], SQSTR["a1"])

        self.game_ply -= 1
 
        self.handler.queuePush(UpdateLogEvent(None, None, self.game_ply, True))
        

    def findPiece(self, uniqueCode: str):
        for key in self.board:
            if self.board[key].uniqueCode == uniqueCode:
                return key

    def drawGUI(self):
        offset_y = int(self.window_height * 0.10)
        offset_x = int(self.board_offset_x * 0.10)

        rect_width = (self.board_offset_x - 2*offset_x)
        rect_height = (self.window_height - 2*offset_y)

        self.gui["offx"] = offset_x
        self.gui["offy"] = offset_y
        self.gui["width"] = rect_width
        self.gui["height"] = rect_height

        gui_rect = RoundRectangle(self.canvas, offset_x, offset_y, offset_x + rect_width, offset_y + rect_height, fill="#262522", tags=("sideboard",))
        dividing_line = self.canvas.create_line(offset_x + int(rect_width * 0.05), offset_y + int(rect_height * 0.10), offset_x + int(rect_width * (0.05 + 0.9)), offset_y + int(rect_height * 0.10), fill="#3C3B39")

        engine_text = self.canvas.create_text(offset_x + int(rect_width * 0.29), offset_y + int(rect_height * 0.058), anchor=tk.E, font=(self.gui_font, 14), text="", fill="#999999", tags=("engine_text"))
    
        #move_log = self.canvas.create_text()

    def drawBoard(self):
        self.canvas.delete("all")
        
        color = self.board_color_dark
        for row in range(8):
            color = self.board_color_light if color == self.board_color_dark else self.board_color_dark
            for col in range(8):
                x1 = self.board_offset_x + (col * self.size) 
                y1 = self.board_offset_y + (row * self.size) 
                x2 = x1 + self.size
                y2 = y1 + self.size
                self.canvas.create_rectangle(x1, y1, x2, y2, outline="white", fill=color, tags=("sq%02dn" % ((7 - row)*8 + col), "square"))
                color = self.board_color_light if color == self.board_color_dark else self.board_color_dark
        
        for sq, piece in self.board.items():
            self.drawPiece(piece, sq)
   

        if self.promotion_ui:
            self.showPromotionUI()

        self.canvas.tag_lower("square")
        self.canvas.tag_raise("piece")
        
    def loadBoard(self, fen: str = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq"):
        self.board.clear()
        self.moves.clear()
        self.move_log.clear()

        self.selected_piece = None
        self.promotion_ui = False
        self._control_variables = {
            "engine_calculating": False, 
            "movelog_count": 2, 
            "movelog_list": [], 
            "drag_calls": 0, 
            "restore": False, 
            "promotion_sq": None
        }

        self.engine.set(fen)
        self.initBoard(fen)
        
        self.drawBoard()
        self.drawGUI()

        if self.selected_piece:
            self.resetSelectedColor()
            self.resetSelectedMovesColor()


    # Load a board from a FEN notation
    def initBoard(self, fen: str):
        self.board.clear()

        spacePos = fen.find(" ")
        if fen[spacePos+1] == "b":
            self.start_player = BLACK
            self.current_player = BLACK
        else:
            self.start_player = WHITE
            self.current_player = WHITE

        square = 56
        for i in range(len(fen)):
            if fen[i] == "/":
                square -= 16
            elif fen[i].isdigit():
                square += int(fen[i])
            elif fen[i] in fenToPiece and i < spacePos:
                self.createPiece(square, fenToPiece[fen[i]][0], fenToPiece[fen[i]][1])
                square += 1

    # Current board to FEN notation
    def toFen(self):
        fen_str = self.engine.fen()

        return fen_str

    # During each frame, redraw each piece
    def drawPiece(self, piece: Piece, square: int):
        x0 = (((square % 8)) * self.size) + int(self.size/2) + self.board_offset_x
        y0 = self.parent.winfo_height() - (((square // 8)) * self.size) - int(self.size/2) - self.board_offset_y

        self.canvas.create_image(x0, y0, image=piece.image, tags=(piece.uniqueCode, "piece"), anchor=tk.CENTER)

    def _moveItem(self, id_or_tag, dx, dy, frames):
        for i in range(frames):
            self.canvas.move(id_or_tag, dx/float(frames), dy/float(frames))
            self.canvas.update()


    def movePiece(self, from_sq: int, to_sq: int):
        self.board[to_sq] = self.board.pop(from_sq)

        x0 = ((from_sq % 8) * self.size) + int(self.size/2) + self.board_offset_x
        y0 = ((7 - (from_sq // 8)) * self.size) + int(self.size/2) + self.board_offset_y
        x1 = ((to_sq % 8) * self.size) + int(self.size/2) + self.board_offset_x
        y1 = ((7 - (to_sq // 8)) * self.size) + int(self.size/2) + self.board_offset_y

        frame_time = int(self.application_fps/2)
        
        self._moveItem(self.board[to_sq].uniqueCode, x1 - x0, y1 - y0, frame_time)
    

    # Given an old position and a new one, place the piece in oldPos in newPos
    def playMove(self, move: Move, move_piece: int = MOVE, sound: bool = True):

        # TODO 1: add images of the eaten piece under or over the 
        
        from_sq = move.getFrom()
        to_sq = move.getTo()
        flags = move.getFlags()
        
        self.game_ply += 1

        if flags == OO:
            self.move_log.append((move, None))
            if self.current_player == BLACK:
                self.movePiece(SQSTR["h8"], SQSTR["f8"])
            else:
                self.movePiece(SQSTR["h1"], SQSTR["f1"])

        if flags == OOO:
            self.move_log.append((move, None))
            if self.current_player == BLACK:
                self.movePiece(SQSTR["a8"], SQSTR["d8"])
            else:
                self.movePiece(SQSTR["a1"], SQSTR["d1"])

        if flags == EN_PASSANT:
            if self.current_player == BLACK:
                captured_pawn = self.board.pop(to_sq + 8)
            else:
                captured_pawn = self.board.pop(to_sq - 8)

            self.move_log.append((move, captured_pawn))
            self.canvas.delete(captured_pawn.uniqueCode)  

        if flags == CAPTURE or flags >= PC_KNIGHT:
            captured_piece = self.board.pop(to_sq)
            self.move_log.append((move, captured_piece))
            self.canvas.delete(captured_piece.uniqueCode)
        
        if flags == QUIET or flags == DOUBLE_PUSH or (flags >= PR_KNIGHT and flags < PC_KNIGHT and flags != CAPTURE and flags != EN_PASSANT):
            self.move_log.append((move, None))

        if move_piece == MOVE:
            self.movePiece(from_sq, to_sq)
        else:
            self.board[to_sq] = self.board.pop(from_sq)
            x1 = ((to_sq % 8) * self.size) + int(self.size/2) + self.board_offset_x
            y1 = ((7 - (to_sq // 8)) * self.size) + int(self.size/2) + self.board_offset_y
            self.canvas.coords(self.board[to_sq].uniqueCode, x1, y1)

        if flags >= PR_KNIGHT and flags != CAPTURE and flags != EN_PASSANT:
            self.promotePawn(to_sq, self.current_player, flagToType(flags))

        if sound:
            if flags == CAPTURE or flags >= PC_KNIGHT:
                mixer.music.load(f"{dir_path}\\assets\\capture.mp3")
                mixer.music.play()
            else:
                mixer.music.load(f"{dir_path}\\assets\\move-self.mp3")
                mixer.music.play() 

        self.engine.move(self.current_player, move)

        self.current_player ^= 1
        self.handler.queuePush(UpdateLogEvent(self.board[to_sq].fentype, move, self.game_ply))

    def waitPromotion(self):
        while self.promotion_ui:
            self.canvas.update()
            self.canvas.update_idletasks()

    # Returns the clicked-on tile's coordinates 
    def getMouseClickPos(self, event) -> tuple:
        try:
            item_below = self.canvas.find_overlapping(event.x,event.y,event.x,event.y)[0]

            return (int(self.canvas.itemcget(item_below, "tags")[2:4]))
        except:
            return SQSTR["None"]

    def setTileColor(self, tile: int, color: str):
        self.canvas.itemconfig(self.canvas.find_withtag("sq%02dn" % (tile))[0], fill=color)

    def resetSelectedColor(self):
        if (self.selected_piece // 8) % 2 == 0:
            self.setTileColor(self.selected_piece, self.board_color_dark) if (self.selected_piece % 8) % 2 == 0 else self.setTileColor(self.selected_piece, self.board_color_light)
        else:
            self.setTileColor(self.selected_piece, self.board_color_light) if (self.selected_piece % 8) % 2 == 0 else self.setTileColor(self.selected_piece, self.board_color_dark)

    def resetSelectedMovesColor(self, sq_array: list):
        for dest in sq_array:
            if (dest // 8) % 2 == 0:
                self.setTileColor(dest, self.board_color_dark) if (dest % 8) % 2 == 0 else self.setTileColor(dest, self.board_color_light)
            else:
                self.setTileColor(dest, self.board_color_light) if (dest % 8) % 2 == 0 else self.setTileColor(dest, self.board_color_dark)

    def setSelectedMovesColor(self, sq_array: list):
        for dest in sq_array:
            if (dest // 8) % 2 == 0:
                self.setTileColor(dest, alpha_blend({self.board_color_dark: 0.8, "#edda95": 0.8})) if (dest % 8) % 2 == 0 else self.setTileColor(dest, alpha_blend({self.board_color_light: 0.8, "#edda95": 0.8}))
            else:
                self.setTileColor(dest, alpha_blend({self.board_color_light: 0.8, "#edda95": 0.8})) if (dest % 8) % 2 == 0 else self.setTileColor(dest, alpha_blend({self.board_color_dark: 0.8, "#edda95": 0.8}))    

    def findMove(self, moves: list, from_sq: int = None, to_sq: int = None, flags: int = None) -> Move:
        mask = 0

        if to_sq != None:
            mask |= (0b111111 << 0)
        else:
            to_sq = 0

        if from_sq != None:
            mask |= (0b111111 << 6) 
        else:
            from_sq = 0

        if flags != None:
            mask |= (0b001111 << 12)
        else:
            flags = 0

        if mask == 0:
            return Move(0)

        for move in moves:
            if (move.getInternal() & mask) == ((flags << 12) | (from_sq << 6) | to_sq):
                return move
            
        return Move(0)

    def onRelease(self, event):
        if self._control_variables["drag_calls"] > 10 and self.selected_piece != None:
            destinations = [move.getTo() for move in self.moves]
            drop_sq = self.getMouseClickPos(event)
            
            if drop_sq in destinations:
                self.resetSelectedColor()
                self.resetSelectedMovesColor(destinations)

                move = self.findMove(self.moves, self.selected_piece, drop_sq)
                if (move.getFlags() >= PR_KNIGHT and (move.getFlags() != CAPTURE and move.getFlags() != EN_PASSANT)):
                    if drop_sq in self.board:
                        self.canvas.delete(self.board[drop_sq].uniqueCode)

                    x1 = ((drop_sq % 8) * self.size) + int(self.size/2) + self.board_offset_x
                    y1 = ((7 - (drop_sq // 8)) * self.size) + int(self.size/2) + self.board_offset_y
                    self.canvas.coords(self.board[self.selected_piece].uniqueCode, x1, y1)

                    self._control_variables["promotion_sq"] = drop_sq
                    self.showPromotionUI()

                    self.waitPromotion()

                    move.setFlags(typeToFlag(self.promotion_type, drop_sq in self.board))
                    

                self.playMove(move, NOMOVE)

                if self.use_ai:
                    self.requestAIMove()

                self.moves.clear()
                self.selected_piece = None 
                return
            
        self._control_variables["drag_calls"] = 0
        if self.selected_piece in self.board:
            x1 = ((self.selected_piece % 8) * self.size) + int(self.size/2) + self.board_offset_x
            y1 = ((7 - (self.selected_piece // 8)) * self.size) + int(self.size/2) + self.board_offset_y
            self.canvas.coords(self.board[self.selected_piece].uniqueCode, x1, y1)
        
    def onDrag(self, event):
        self._control_variables["drag_calls"] += 1

        try:
            if self.selected_piece != None and self._control_variables["drag_calls"] > 5:
                self.canvas.tag_raise(self.board[self.selected_piece].uniqueCode)
                self.canvas.coords(self.board[self.selected_piece].uniqueCode, event.x, event.y)
                pass
        except:
            pass

    # What to do on clicks        
    def onClick(self, event):
        if self.selected_piece == None:
            click_sq = self.getMouseClickPos(event)

            if click_sq in self.board and self.board[click_sq].color == self.current_player:
                self.selected_piece = click_sq
                
                
                self.moves = self.generateLegalMoves(click_sq)
                destinations = [move.getTo() for move in self.moves]
                self.setTileColor(self.selected_piece, self.selected_piece_color)

                self.setSelectedMovesColor(destinations)
        else:
            click_sq = self.getMouseClickPos(event)
            destinations = [move.getTo() for move in self.moves]

            # If the user clicks on another piece
            if click_sq not in destinations and click_sq in self.board and self.board[click_sq].color == self.current_player:
                self.resetSelectedColor()

                self.resetSelectedMovesColor(destinations)

                self.selected_piece = click_sq
                self.setTileColor(click_sq, self.selected_piece_color)
                    
                
                self.moves = self.generateLegalMoves(self.selected_piece)
                destinations = [move.getTo() for move in self.moves]
            
                
                self.setSelectedMovesColor(destinations)

            # If the user clicks on a move
            elif click_sq in destinations:
                self.resetSelectedColor()
        
                self.resetSelectedMovesColor(destinations)

                move = self.findMove(self.moves, self.selected_piece, click_sq)

                if (move.getFlags() >= PR_KNIGHT and (move.getFlags() != CAPTURE and move.getFlags() != EN_PASSANT)):
                    self._control_variables["promotion_sq"] = click_sq
                    self.showPromotionUI()

                    self.waitPromotion()

                    move.setFlags(typeToFlag(self.promotion_type, click_sq in self.board))

                self.playMove(move)
                self.moves.clear()
                self.selected_piece = None

                if self.use_ai:
                    self.requestAIMove()
                
            # If the user clicks on an empty square
            else:
                self.resetSelectedColor()
                self.resetSelectedMovesColor(destinations)
                self.selected_piece = None

    def generateAllLegalMoves(self, side_color: int):
        move_codes = self.engine.generate_moves((100 + side_color))

        moves = arrayU16toMoves(move_codes)

        return moves

    def inCheck(self, color: int) -> bool:
        return self.engine.in_check(color)           

    def generateLegalMoves(self, square: int) -> list:
        if square > 63 or square < 0:
            return []

        moves = self.engine.generate_moves(square)
        
        return moves
    
    def onPromotionClick(self, event):
        try:
            clicked_square = int(self.canvas.itemcget(self.canvas.find_overlapping(event.x,event.y,event.x,event.y)[0], "tags")[2])
        except:
            return
    
        if clicked_square == 0:
            self.promotion_type = QUEEN
        elif clicked_square == 1:
            self.promotion_type = KNIGHT
        elif clicked_square == 2:
            self.promotion_type = ROOK
        elif clicked_square == 3: 
            self.promotion_type = BISHOP      
        else:
            return
        
        self.canvas.delete("promotion")
        self.canvas.bind("<Button-1>", self.onClick)
        self.promotion_ui = False

    def showPromotionUI(self):
        self.canvas.unbind("<Button-1>")
        self.canvas.bind("<Button-1>", lambda event: self.onPromotionClick(event))
        self.promotion_ui = True
        sq_size = int(self.size * 0.9)
        
        if self.current_player == WHITE:
            for i in range(4):
                x1 = self.board_offset_x + (i * sq_size)
                y1 = self.board_offset_y - sq_size - 5

                x0 = self.board_offset_x + (i * sq_size) + (sq_size//2)
                y0 = y1 + (sq_size//2)
                
                x2 = x1 + sq_size
                y2 = y1 + sq_size
                self.canvas.create_rectangle(x1, y1, x2, y2, outline="white", fill=self.board_color_light, tags=(f"pr{i}", "promotion", "square"))
                self.canvas.create_image(x0, y0, image=self.fenToImage[list(self.fenToImage.keys())[7+i]], tags=("promotion", "piece"), anchor="c")
                self.canvas.tag_raise("piece")
                self.canvas.tag_lower("square")
        if self.current_player == BLACK:
            for i in range(4):
                x1 = self.board_offset_x + (i * sq_size)
                y1 = self.board_offset_y + 5 + (8 * self.size)
                x2 = x1 + sq_size
                y2 = y1 + sq_size

                x0 = self.board_offset_x + (i * sq_size) + (sq_size//2)
                y0 = y1 + (sq_size//2)
    
                self.canvas.create_rectangle(x1, y1, x2, y2, outline="white", fill=self.board_color_light, tags=(f"pr{i}", "promotion", "square"))
                self.canvas.create_image(x0, y0, image=self.fenToImage[list(self.fenToImage.keys())[1+i]], tags=("promotion", "piece"), anchor="c")
                self.canvas.tag_raise("piece")
                self.canvas.tag_lower("square")

    def promotePawn(self, pawn_sq: int, player_color: int, promotion_type: int):
        self.canvas.delete(self.board[pawn_sq].uniqueCode)
        self.createPiece(pawn_sq, promotion_type, player_color)
        self.drawPiece(self.board[pawn_sq], pawn_sq)
        
if __name__ == "__main__":
    #start="rnbqkbnr/1pppppPp/8/8/8/8/PpPPPPP1/RNBQKBNR w KQkq"
    #start="rnbqkbnr/pppp1ppp/4p3/8/1P6/N1P5/P2PPPPP/R1BQKBNR w KQkq"
    board = Board(start=START, debug = True, ai = True);
    board.mainloop()
