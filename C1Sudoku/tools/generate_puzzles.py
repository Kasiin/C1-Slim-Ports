"""Reproducible, unique-solution Sudoku bank; no downloaded puzzles."""
import json
import random
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
UNITS = ([[r * 9 + c for c in range(9)] for r in range(9)] +
         [[r * 9 + c for r in range(9)] for c in range(9)] +
         [[(r + y) * 9 + c + x for y in range(3) for x in range(3)]
          for r in (0, 3, 6) for c in (0, 3, 6)])
PEERS = [set().union(*(set(u) for u in UNITS if i in u)) - {i} for i in range(81)]

def candidates(b, i):
    return set(range(1, 10)) - {b[p] for p in PEERS[i]}

def count_solutions(board, limit=2):
    b = board[:]
    rows, cols, boxes = [0]*9, [0]*9, [0]*9
    for i, d in enumerate(b):
        if d:
            r, c, k = i//9, i%9, i//27*3+i%9//3
            bit = 1 << d
            if (rows[r] | cols[c] | boxes[k]) & bit:
                return 0
            rows[r] |= bit; cols[c] |= bit; boxes[k] |= bit
    def visit():
        pos, mask, best = -1, 0, 10
        for i, d in enumerate(b):
            if d: continue
            r,c,k = i//9,i%9,i//27*3+i%9//3
            m = 1022 & ~(rows[r] | cols[c] | boxes[k])
            n = m.bit_count()
            if n == 0: return 0
            if n < best:
                pos,mask,best=i,m,n
                if n == 1: break
        if pos < 0: return 1
        r,c,k=pos//9,pos%9,pos//27*3+pos%9//3
        total=0
        while mask:
            bit=mask & -mask; mask-=bit
            b[pos]=bit.bit_length()-1
            rows[r]|=bit;cols[c]|=bit;boxes[k]|=bit
            total+=visit()
            rows[r]^=bit;cols[c]^=bit;boxes[k]^=bit;b[pos]=0
            if total>=limit: break
        return total
    return visit()

def logic_solved(board, hidden=False):
    b=board[:]
    while 0 in b:
        choices={i:candidates(b,i) for i,d in enumerate(b) if not d}
        single=next(((i,next(iter(s))) for i,s in choices.items() if len(s)==1),None)
        if not single and hidden:
            for unit in UNITS:
                for d in range(1,10):
                    cells=[i for i in unit if i in choices and d in choices[i]]
                    if len(cells)==1:
                        single=(cells[0],d);break
                if single:break
        if not single:return False
        b[single[0]]=single[1]
    return True

def make_bank():
    rng=random.Random(20260904)
    bank=[[],[],[]]; seen=set(); attempts=0
    while any(len(x)<200 for x in bank):
        attempts+=1
        groups=list(range(3));rng.shuffle(groups)
        rows=[g*3+j for g in groups for j in rng.sample(range(3),3)]
        rng.shuffle(groups)
        cols=[g*3+j for g in groups for j in rng.sample(range(3),3)]
        digits=rng.sample(range(1,10),9)
        solution=[digits[(r*3+r//3+c)%9] for r in rows for c in cols]
        b=solution[:]
        order=rng.sample(range(81),81)
        # Take snapshots at 40, 32 and 26 clues; retain only the matching
        # logical grade. Each snapshot is independently unique-checked.
        for i in order:
            old=b[i];b[i]=0
            if count_solutions(b)!=1:b[i]=old;continue
            clues=sum(bool(v) for v in b)
            if clues not in (40,32,26):continue
            if logic_solved(b):grade=0
            elif logic_solved(b,True):grade=1
            else:grade=2
            if grade!=(40,32,26).index(clues) or len(bank[grade])>=200:continue
            key=''.join(map(str,b))
            if key in seen:continue
            seen.add(key)
            bank[grade].append({'puzzle':key,'solution':''.join(map(str,solution))})
            if len(bank[grade])%25==0:print('bank',list(map(len,bank)),'attempts',attempts,flush=True)
            if all(len(x)==200 for x in bank):break
            if clues==26:break
    return bank

def verify(bank):
    assert list(map(len,bank))==[200]*3
    seen=set()
    for level,items in enumerate(bank):
        for item in items:
            p=list(map(int,item['puzzle']));s=list(map(int,item['solution']))
            assert len(p)==len(s)==81
            assert all(not a or a==b for a,b in zip(p,s))
            assert all({s[i] for i in u}==set(range(1,10)) for u in UNITS)
            assert item['puzzle'] not in seen
            seen.add(item['puzzle'])
            assert count_solutions(p)==1
            assert (0 if logic_solved(p) else 1 if logic_solved(p,True) else 2)==level

if __name__=='__main__':
    out=ROOT/'assets';out.mkdir(exist_ok=True,parents=True)
    path=out/'puzzles.json'
    bank=json.loads(path.read_text()) if path.exists() else make_bank()
    verify(bank)
    path.write_text(json.dumps(bank,separators=(',',':'))+'\n')
    header='// Generated and verified by tools/generate_puzzles.py\nstatic const char *PUZZLES[3][200][2] = {\n'
    for level in bank:
        header+='{\n'+''.join('{"'+x['puzzle']+'","'+x['solution']+'"},\n' for x in level)+'},\n'
    (out/'puzzles.h').write_text(header+'};\n')
    print('Verified 600 distinct, uniquely solvable puzzles; grades: naked singles / hidden singles / beyond singles.',flush=True)
