from typing import List

# Move related constants
QUIET = 0b0000
DOUBLE_PUSH = 0b0001
OO = 0b0010
OOO = 0b0011
CAPTURE = 0b1000
CAPTURES = 0b1111
EN_PASSANT = 0b1010
PROMOTIONS = 0b0111
PROMOTION_CAPTURES = 0b1100
PR_KNIGHT = 0b0100
PR_BISHOP = 0b0101
PR_ROOK = 0b0110
PR_QUEEN = 0b0111
PC_KNIGHT = 0b1100
PC_BISHOP = 0b1101
PC_ROOK = 0b1110
PC_QUEEN = 0b1111

NOMOVE = 0xFF

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

MOVE_TYPESTR_B = [
	"", "", " O-O", " O-O-O", " N (promotion)", " B (promotion)", " R (promotion)", " Q (promotion)", " (capture)", "", " e.p.", "",
	" N (promotion)", " B (promotion)", " R (promotion)", " Q (promotion)"
];

MOVE_FLAGSTR = [
    "", "", "OO", "OOO", "", "", "", "", "x", "", "ep", "", "", "", "", ""
]

class Move:
    def __init__(self, move_internal: int = -1, from_sq: int = 0, to_sq: int = 0, flags: int = 0):
        if (move_internal != -1):
            self.move = move_internal
        else:
            self.move = int((flags << 12) | (from_sq << 6) | to_sq);
    
    def getFrom(self):
        return (self.move >> 6) & 0x3f

    def getTo(self):
        return self.move & 0x3f
    
    def getFlags(self):
        return self.move >> 12
    
    def setFlags(self, flags: int):
        self.move &= (0b111111 << 6 | 0b111111)
        self.move |= (flags << 12)
    
    def toBytes(self):
        return self.move.to_bytes(2, "little")
    
    def toString(self):
        return SQSTR[self.getFrom()] + SQSTR[self.getTo()] + MOVE_TYPESTR_B[self.getFlags()]
    
    def getInternal(self):
        return self.move
    
    def __str__(self):
        return_string = ""

        for square_string, square in SQSTR.items():
            if square == self.getFrom():
                return_string += square_string

        for square_string, square in SQSTR.items():    
            if square == self.getTo():
                return_string += square_string 

        return return_string + MOVE_TYPESTR_B[self.getFlags()]
    
def arrayU16toMoves(buffer: bytearray) -> List[Move]:
    moves = []

    if int.from_bytes(buffer[:2], "little") == 0:
            return moves

    for i in range(0, len(buffer), 2):
        moves.append(Move(int.from_bytes(buffer[i:i + 2], "little")))

    return moves
