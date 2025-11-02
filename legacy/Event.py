from enum import Enum
from Move import *
import queue

WINDOW_RESIZE_EVENT = 0
MOUSE_PRESSED_EVENT = 1
MOUSE_RELEASED_EVENT = 2
KEYBOARD_PRESSED_EVENT = 3
KEYBOARD_RELEASED_EVENT = 4
ENGINE_MOVE_EVENT = 5
UPDATE_LOG_EVENT = 6

class Event:
    def __init__(self):
        self.type = None

class Handler:
    def __init__(self):
        self.handle_list: dict = {}
        self.event_queue: queue.Queue = queue.Queue()

    def queuePush(self, event: Event):
        self.event_queue.put_nowait(event)

    def queuePop(self) -> Event:
        if self.event_queue.empty():
            return None

        return self.event_queue.get_nowait()

    def addHandle(self, ev_type, func):
        self.handle_list[ev_type] = func

    def dispatchEvent(self, event: Event):
        if event.type not in self.handle_list:
            return

        self.handle_list[event.type](event)
    
    def pollEvents(self):
        while ((e := self.queuePop()) != None):
            self.dispatchEvent(e)

class WindowResizeEvent(Event):
    def __init__(self, new_width, new_height):
        self.type = WINDOW_RESIZE_EVENT
        self.width = new_width
        self.height = new_height

class EngineMoveEvent(Event):
    def __init__(self, move: Move = None, move_code: int = 0):
        self.type = ENGINE_MOVE_EVENT
        self.move_code = move_code

        self.move = move

        if (self.move_code != 0):
            self.move = Move(self.move_code)


class UpdateLogEvent(Event):
    def __init__(self, fen_type, move, ply = 0, restore: bool = False):
        self.type = UPDATE_LOG_EVENT
        self.move = move
        self.ply = ply
        self.fen_type = fen_type
        self.restore = restore

class MousePressedEvent(Event):
    def __init__(self, x, y, button = 1):
        self.type = MOUSE_PRESSED_EVENT
        self.x = x
        self.y = y
        self.button = button

class MouseReleasedEvent(Event):
    def __init__(self, x, y, button = 1):
        self.type = MOUSE_RELEASED_EVENT
        self.x = x
        self.y = y
        self.button = button

class KeyboardPressedEvent(Event):
    def __init__(self, key_code):
        self.type = KEYBOARD_PRESSED_EVENT
        self.code = key_code

class KeyboardReleasedEvent(Event):
    def __init__(self, key_code):
        self.type = KEYBOARD_RELEASED_EVENT
        self.code = key_code