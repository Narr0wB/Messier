from EngineAPI import *

if __name__ == "__main__":
    initialize_engine_dll("build/Engine.dll")

    engine1 = Engine()
    engine2 = Engine()

    # engine1.set("8/5p2/1p6/2k5/5Rp1/K5P1/q5r1/8 w - - 2 2") "Engine/x64/Release/Engine.exe" "8/8/8/8/3B4/2Q5/8/3k2K1 w - -"

    # THE SOLUTION IS TO REMOVE FROM THE EVALUABLE MOVES THE LAST PLAYED MOVE
    # r1bqkb1r/pppn1ppp/3p1n2/4p3/3P4/2N1P2P/PPP2PP1/R1BQKBNR w KQkq - 1 5

    # print(engine1.checkmate(WHITE))
    engine1.set(START)
    engine2.set(START)

    engine1_color = WHITE
    engine2_color = BLACK

    debug = True
    flag = 0

    while ( condition := (not engine1.checkmate(engine1_color)) and (not engine2.checkmate(engine2_color)) ):
        #print(engine1.fen())
        print("Engine v0 to move")
        move = engine1.ai_move(4, engine1_color)

        if debug:
            print(f"Engine v0 played {move}")
        
        if (move.getInternal() == 0):
            print(engine1.fen())
            break
        
        engine1.move(engine1_color, move)
        engine2.move(engine1_color, move)

        if (not (condition := (not engine1.checkmate(engine1_color)) and (not engine2.checkmate(engine2_color)))):
            break

        print("Engine v0.1 to move")
        move = engine2.ai_move(4, engine2_color)

        if debug:
            print(f"Engine v0.1 played {move}")

        engine1.move(engine2_color, move)
        engine2.move(engine2_color, move)

    print(f"{COLOR_STR_B[engine1_color] if engine2.checkmate(engine2_color) else COLOR_STR_B[engine2_color]} won!")
    print(engine1.fen())
    print(engine2.fen())

    engine1.exit()
    engine2.exit()
