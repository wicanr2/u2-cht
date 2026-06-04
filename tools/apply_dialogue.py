#!/usr/bin/env python3
"""填入 talk_dialogue.tsv 的 zh_hant 欄。

NPC 對話含 1980 年代業界/流行文化彩蛋 (Richard Garriott、Robert Woodhead、
Hotel California、San Jose 歌、Sister Sledge…) → 譯出意思,保留趣味。
重疊 buffer 殘片 (如 "AN JOSE?"、"T HERE!"、" A BAD THING...") 留空。
\\r 為段內換行,保留。

用法: python3 apply_dialogue.py translations/talk_dialogue.tsv
"""
import sys

T = {
    r"THE SHE CREATURE BEGS:\rTAKE ME I'M YOURS!": r"雌性生物哀求:\r帶我走,我是你的!",
    r"HOMER THE SALESMAN ASKS:\rYA WANT SOME LIFE INSURANCE?": r"推銷員荷馬問:\r想買點壽險嗎?",
    r"DAVID ALPERT WHIMPERS:\rI'VE DONE A BAD THING...": r"大衛.阿爾伯特啜泣:\r我做了壞事……",
    r"MINAX CRIES: A COWARD DIES A\rTHOUSAND DEATHS, A HERO ONLY\rONE. EITHER WAY, YOU LOSE!!!":
        r"米娜克斯尖叫:懦夫死千次,\r英雄只死一次。無論如何,\r你都輸定了!!!",
    r"AN ASTRONAUT CLAIMS:\rTHERE IS A PLANET 'X'!": r"太空人聲稱:\r有一顆行星叫『X』!",
    r"AN ASTRONAUT CLAIMS:\rFATHER ANTOS LIVES ON 'X'": r"太空人聲稱:\r安托斯神父住在『X』上",
    r"THE DRUID: ANOL NATHRAC UTH\rDAS BESSOD DIEN DOCH DIENTES!":
        r"德魯伊:ANOL NATHRAC UTH\rDAS BESSOD DIEN DOCH DIENTES!",
    r"THE PHYLOSOPHER THEORIZES:\rSHE'S GOT TO BE AT 9-9-9!": r"哲學家推論:\r她一定在 9-9-9!",
    r"GRENDEL THE BUM SAYS: MASTERS\rOF RIDDLE ARE MASTERS INDEED!":
        r"流浪漢格倫德說:精通謎題者\r方為真正的高手!",
    r"ANDY GREENBERG COMPLAINS:\rWHAT? NO SOFTWARE?!?": r"安迪.格林堡抱怨:\r什麼?沒有軟體?!?",
    r"ROBERT WOODHEAD EXCLAIMS:\rCOPY PROTECT! COPY PROTECT!": r"羅伯.伍德海大喊:\r防拷!防拷!",
    r"THE CHASTE NUBILE NYMPH ASKS:\rVISIT THE HOTEL CALIFORNIA!": r"純真的妙齡仙女問:\r去加州旅館看看吧!",
    r"ITHILIAN THE WENCH CALLS:\rMAKE ME AN OFFER SAILOR!": r"酒館女郎伊希莉安招呼:\r出個價吧,水手!",
    r"JOHN MAYER ORDERS:\rTAKE OUT THE TRASH!!!": r"約翰.梅爾命令:\r把垃圾拿出去!!!",
    r"GERRY MAYER PROCLAIMS:\rWE KNOW SMALL COMPUTERS!": r"傑瑞.梅爾宣稱:\r我們最懂小型電腦!",
    r"KEITH SAYS: NO THANKS,\rTRYING TO CUT DOWN.": r"基斯說:不了,謝謝,\r我正在節制。",
    r"BROTHER ANTOS ORDAINS:\rSEARCH THE STARS FOR MY KIN!": r"安托斯修士諭示:\r到群星間尋找我的同族!",
    r"BISHOP BOB PREACHES:\rCONFESS TO BROTHER ANTOS!": r"鮑伯主教佈道:\r向安托斯修士懺悔吧!",
    r"ANDRE THE FRENCH CHEF BURPS:\rI DRINK THEREFORE, I AM!": r"法國廚師安德烈打嗝:\r我飲故我在!",
    r"COMMANDER DECKER SCREAMS:\rIS IT SOUP YET!!!": r"戴克指揮官大叫:\r湯好了沒!!!",
    r"THE WANDERING BARD ASKS: DO\rYOU KNOW THE WAY TO SAN JOSE?": r"流浪吟遊詩人問:你知道\r往聖荷西的路嗎?",
    r"LADY SHERRIE CRIES:\rGAG ME WITH A SPOON!": r"雪莉小姐嚷道:\r噁心死我了!",
    r"RONALL MC DONALL SINGS:\rTRY OUR NEW RIDE THROUGH!": r"羅納.麥當勞唱道:\r來試試我們的新得來速!",
    r"THE SIGHTLESS SAGE SAYS: FIND\rTHE FATHER EARN THE RING!": r"失明的智者說:找到\r那位神父,贏得戒指!",
    r"LADY MARY ELLEN PLEADS:\rBEAT ME KICK ME TELL ME LIES!": r"瑪麗.艾倫小姐懇求:\r打我踢我對我說謊吧!",
    r"SANTRE THE SWASHBUCKLER WARNS:BEWARE, I'VE A QUICK BLADE!": r"劍客桑特警告:當心,我的劍可快了!",
    r"AN OLD MAN MUMBLES:\rI'M AN OLD MAN.": r"一位老人喃喃:\r我是個老頭子。",
    r"THE FLOPY FIGHTER BRAGS:\rI OWN ONE OF EVERYTHING!": r"軟趴趴的戰士吹噓:\r我每樣都有一件!",
    r"A CLERK STATES: WELCOME\rTO THE HOTEL CALIFORNIA!": r"櫃員說:歡迎光臨\r加州旅館!",
    r"BROTHER ANTOS INQUIRES:\rHAVE YOU FOUND MY FATHER?": r"安托斯修士詢問:\r你找到我父親了嗎?",
    r"SISTER SLEDGE SINGS:\rWE ARE FAMILY!": r"史蕾姊妹合唱:\r我們是一家人!",
    r"ANDRE THE FRENCH CHEF SAYS:\rHIC, WAN SOM COLDUCK, HIC?": r"法國廚師安德烈說:\r嗝,要來點冷鴨嗎,嗝?",
    r"THE MERCHANT ORDERS:\rGIVE THE KING A TRIBUTE!": r"商人吩咐:\r給國王獻上貢品!",
    r"THE SUBDUED BALRON WHIMPERS:\rLITHIUM PACKS THE 16TH LEVEL!": r"被制伏的炎魔啜泣:\r三鋰滿佈在第 16 層!",
    r"WAREN BEATTY ASKS:\rHAVE YOU SEEN DIANE KEATON?": r"華倫.比提問:\r你看到黛安.基頓了嗎?",
    r"CHUCKLES THE BUMBLE CRIES:\rCAN I LOOK YET, IS IT OVER?": r"笨拙的恰克喊道:\r我能看了嗎,結束了沒?",
    r"THE VILLAGE IDIOT SUGGESTS:\rOFFER THE HOTEL CLERC GOLD!": r"村裡的傻子建議:\r給旅館櫃員獻上黃金!",
    r"RED THE PIRATE BOASTS: I'VE A\rBROKEN ULTIMA ]I[, WANT ONE?": r"紅毛海盜誇口:我有一片\r壞掉的創世紀 ]I[,要嗎?",
    r"THE SWAMP JESTER LAUGHS:\rYOU LOSE, CADET!": r"沼澤弄臣大笑:\r你輸了,菜鳥!",
    r"THE COURT JESTER SAYS:\rISN'T THIS A SILLY PLACE?": r"宮廷弄臣說:\r這地方是不是很蠢?",
    r"THE FRENCH JESTER BEGS:\rMANGER MOI!": r"法國弄臣懇求:\rMANGER MOI!(吃我吧!)",
    r"THE STONED JESTER COMPLAINS:\rROCKS HAVE SPIRITS, TOO!": r"嗑藥的弄臣抱怨:\r石頭也有靈魂啊!",
    r"DEBBIE THE LIFEGUARD YELLS:\rGET A BIG GRIP!": r"救生員黛比大喊:\r振作點!",
    r"RICHARD GARRIOTT PROMISES:\rTOMORROW--FOR SURE!": r"理查.蓋瑞特保證:\r明天──一定好!",
    r"HOWIE THE PEST HOUNDS:\rISN'T ULTIMA ][ FINISHED YET?": r"煩人精豪伊追問:\r創世紀 ][ 還沒做完嗎?",
    r"JOHNATHON BEN ASKS:\rTHIS EVER HAPPEN TO YOU?": r"強納森.班問:\r你也遇過這種事嗎?",
    r"MARGOT TOMMERVIK EXCLAIMS:\rTALK SOFTLY TO ME!": r"瑪歌.湯默維克喊道:\r溫柔地對我說話!",
    r"AL TOMMERVIK INQUIRES:\rDO YOU READ ME?": r"艾爾.湯默維克問:\r你收到我訊息了嗎?",
    r"BIG BOB JAWS:\rGIVE BILL 300 GOLD!": r"大塊頭鮑伯咆哮:\r給比爾 300 黃金!",
    r"BILL BRUISER DEMANDS:\rGIVE BOB 600 GOLD!": r"惡棍比爾要求:\r給鮑伯 600 黃金!",
    r"THE SEAWORTHY PIRATE SAYS:\rSEE THE CLERK IN NEW SAN.": r"耐航的海盜說:\r去找新聖城的櫃員。",
    r"THE BURLY PIRATE SAYS:\rNEW SAN. IS FULL OF MAGIC!": r"魁梧的海盜說:\r新聖城充滿魔法!",
    r"THE UGLY SMELLY PIRATE SAYS:\rANTOS IS THE ANSWER!": r"又醜又臭的海盜說:\r安托斯就是答案!",
    r"THE BIG BOUNCER STALLS:\rWHERE THE HELL IS YOUR I.D.?": r"彪形保鏢攔下你:\r你的證件到底在哪?",
    r"UGLY IRVING STATES:\rNO MAGES ALLOWED!": r"醜歐文表明:\r巫師不得進入!",
    r"KIETH ZABALAOUI SAYS:\rI'VE NEVER DONE THIS BEFORE!": r"基斯.札巴拉維說:\r我以前從沒做過這個!",
    r"AN EXPERIENCED WARRIOR SAYS:\rFIND ANTOS, EARN THE RING!": r"老練的戰士說:\r找到安托斯,贏得戒指!",
    r"A WIMPY WARRIOR SQUEALS:\rANTOS IS ON 'X'!": r"窩囊的戰士尖叫:\r安托斯在『X』上!",
    r"FATHER ANTOS CHANTS:\rYOU HAVE EARNED MY BLESSING.\rRETURN AND CLAIM THE RING!":
        r"安托斯神父吟誦:\r你已贏得我的祝福。\r回去領取那枚戒指吧!",
    r"SING LEE THE COOK YELLS:\rKICHEN CLOST, COM BAC LATO!": r"廚子星李大喊:\r廚房打烊,晚點再來!",
    r"JUSTIN THE JAILER WARNS:\rI WOULDN'T GO IN THERE!": r"獄卒賈斯汀警告:\r我才不會進去那裡!",
    r"QUEEN SUSAN SAYS:\rFATHER ANTOS AWAITS YOU!": r"蘇珊王后說:\r安托斯神父在等你!",
}


def main():
    path = sys.argv[1]
    lines = open(path, encoding="utf-8").read().splitlines()
    out = [lines[0]]
    done = frag = 0
    for ln in lines[1:]:
        f = ln.split("\t")
        while len(f) < 4:
            f.append("")
        orig = f[2]
        zh = T.get(orig, "")
        if zh:
            done += 1
        else:
            frag += 1
        out.append("\t".join([f[0], f[1], f[2], zh]))
    open(path, "w", encoding="utf-8").write("\n".join(out) + "\n")
    print(f"對話: {done} 句已譯 / {frag} 殘片或重複留空")


if __name__ == "__main__":
    main()
