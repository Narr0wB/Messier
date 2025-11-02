import tkinter as tk
import math

def dist(p1: tuple, p2: tuple) -> float:
    return math.sqrt(math.pow(p1[0] - p2[0], 2)+ math.pow(p1[1] - p2[1], 2))

def alpha_blend(color_dict: dict) -> str:
    colors = sorted(color_dict.items())
    w_sum = sum(color_dict.values())

    # Weighted Average between the different colors' rgb values

    red = int(sum(int(x.replace("#", "")[:2], 16)*w for x, w in colors)/w_sum)
    green = int(sum(int(x.replace("#", "")[2:4], 16)*w for x, w in colors)/w_sum)
    blue = int(sum(int(x.replace("#", "")[4:6], 16)*w for x, w in colors)/w_sum)

    # Output the final color

    return "#" + f"{red:02x}" + f"{green:02x}" + f"{blue:02x}"

class HoverPool:
    def __init__(self, canvas: tk.Canvas):
        self.hover_items = {}
        
        self.canvas = canvas
        self.canvas.bind("<Motion>", self.check)

    def add(self, item_tags: tuple, callback):
        self.hover_items[item_tags] = callback

    def check(self, event):
        for tags, cb in self.hover_items.items():
            if tags in self.canvas.gettags("current"):
                cb[0]()
            else:
                cb[1]()

def round(num):
    if (num-math.floor(num) < 0.5):
        return math.floor(num)
    else:
        return math.ceil(num)

class GuiMoveElement():
    def __init__(self, canvas: tk.Canvas, x, y, width, height, ply, white_move = "", black_move = "", **kwargs):
        self.x = x
        self.y = y
        self.width = width
        self.height = height

        self.canvas = canvas
        self.ply = ply
        self.id = f"hsh{ply}"

        self.creation_kwargs = kwargs

        # self.hover_pool = HoverPool(self.canvas)
        # self.hover_pool.add(("white_text"), (self.setWhiteHover, lambda: self.canvas.itemconfig(f"{self.id}&&white_hover", fill=self.canvas.itemcget(self.rect, "fill"))))
        # self.hover_pool.add(("black_text"), (self.setBlackHover, lambda: self.canvas.itemconfig(f"{self.id}&&black_hover", fill=self.canvas.itemcget(self.rect, "fill"))))

        self.rect = self.canvas.create_rectangle(x + int(self.height/2), y - int(self.height/2), x + int(self.height/2) + self.width, y + int(self.height/2) + 1, tags=("move_log", self.id), outline="", **kwargs)
        self.circles = [
            self.canvas.create_oval(x, y - int(self.height/2), x + 2*int(self.height/2), y + int(self.height/2), tags=("move_log", self.id), outline="", **kwargs),
            self.canvas.create_oval(x + width, y - int(self.height/2), x + 2*int(self.height/2) + width, y + int(self.height/2), tags=("move_log", self.id), outline="", **kwargs)
        ]

        # self.hove_rects = [
        #     RoundRectangle(self.canvas, x + int(self.width * 0.3), y - int(self.height * 0.3), x + int(self.width * 0.5), y + int(self.height * 0.3), 8, tags=("move_log", self.id, "white_hover"), **kwargs),
        #     RoundRectangle(self.canvas, x + int(self.width * 0.605), y - int(self.height * 0.3), x + int(self.width * 0.805), y + int(self.height * 0.3), 8, tags=("move_log", self.id, "black_hover"), **kwargs)
        # ]

        self.text = [
            self.canvas.create_text(x + int(self.width * 0.17), y, text=f"{self.ply}", font=("Noto Sans Medium", 12, "bold"), fill="#999999", tags=("move_log", self.id, "ply_text"), anchor=tk.W),
            self.canvas.create_text(x + int(self.width * 0.35), y, text=f"{white_move}", font=("Noto Sans Medium", 12), fill=alpha_blend({"#999999": 1, "#FFFFFF": 0}), tags=("move_log", self.id, "white_text"), anchor=tk.W),
            self.canvas.create_text(x + int(self.width * 0.7), y, text=f"{black_move}", font=("Noto Sans Medium", 12), fill=alpha_blend({"#999999": 1, "#000000": 0}), tags=("move_log", self.id, "black_text"), anchor=tk.W)
        ]
   
    def move(self, dx, dy):
        self.canvas.move(self.id, dx, dy)
    
    def updateWhite(self, text):
        self.white_text = text
        self.canvas.itemconfig(f"{self.id}&&white_text", text=text)

    def updateBlack(self, text):
        self.black_text = text
        self.canvas.itemconfig(f"{self.id}&&black_text", text=text)

    def getId(self):
        return self.id
    
    def setWhiteHover(self):
        self.canvas.itemconfig(f"{self.id}&&white_hover", fill=alpha_blend({"#21201E": 0.5, "#000000": 0.5}))
    
    def setBlackHover(self):
        self.canvas.itemconfig(f"{self.id}&&black_hover", fill=alpha_blend({"#21201E": 0.5, "#000000": 0.5}))
    
    def draw(self, x, y):
        self.rect = self.canvas.create_rectangle(x + int(self.height/2), y - int(self.height/2), x + int(self.height/2) + self.width, y + int(self.height/2) + 1, tags=("move_log", self.id), outline="", **self.creation_kwargs)
        self.circles = [
            self.canvas.create_oval(x, y - int(self.height/2), x + 2*int(self.height/2), y + int(self.height/2), tags=("move_log", self.id), outline="", **self.creation_kwargs),
            self.canvas.create_oval(x + self.width, y - int(self.height/2), x + 2*int(self.height/2) + self.width, y + int(self.height/2), tags=("move_log", self.id), outline="", **self.creation_kwargs)
        ]

        self.text = [
            self.canvas.create_text(x + int(self.width * 0.17), y, text=f"{self.ply}", font=("Noto Sans Medium", 12, "bold"), fill="#999999", tags=("move_log", self.id, "ply_text"), anchor=tk.W),
            self.canvas.create_text(x + int(self.width * 0.35), y, text=self.white_text, font=("Noto Sans Medium", 12), fill=alpha_blend({"#999999": 1, "#FFFFFF": 0}), tags=("move_log", self.id, "white_text"), anchor=tk.W),
            self.canvas.create_text(x + int(self.width * 0.7), y, text=self.black_text, font=("Noto Sans Medium", 12), fill=alpha_blend({"#999999": 1, "#000000": 0}), tags=("move_log", self.id, "black_text"), anchor=tk.W)
        ]


class RoundRectangle():
    def __init__(self, canvas: tk.Canvas, x1, y1, x2, y2, radius = 20, **kwargs):
        self.rect_x1 = x1
        self.rect_y1 = y1
        self.rect_x2 = x2
        self.rect_y2 = y2

        self.circle_radius = radius
        self.canvas = canvas

        offset = math.floor(radius/2)

        rect_points = [
            x1, y1 + offset,
            x1 + offset, y1,
            x2 - offset, y1,
            x2, y1 + offset,
            x2, y2 - offset,
            x2 - offset, y2,
            x1 + offset, y2,
            x1, y2 - offset
        ]

        self.rectangle = canvas.create_polygon(rect_points, **kwargs)
        self.circles = [
            canvas.create_oval(x1, y1, x1 + self.circle_radius, y1 + self.circle_radius, outline="", **kwargs),
            canvas.create_oval(x2 - 1, y1, x2 - self.circle_radius, y1 + self.circle_radius, outline="", **kwargs),
            canvas.create_oval(x2 - 1, y2 - 1, x2 - self.circle_radius, y2 - self.circle_radius, outline="", **kwargs),
            canvas.create_oval(x1, y2 - 1, x1 + self.circle_radius, y2 - self.circle_radius, outline="", **kwargs),
        ]