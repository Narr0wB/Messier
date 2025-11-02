
from termcolor import *

def error(err_msg: str): 
    cprint("[E] " + err_msg, "red")
    # exit(0)

def warn(wrn_msg: str):
    cprint("[W] " + wrn_msg, "yellow")

def info(inf_msg: str):
    cprint("[I] " + inf_msg, "green")