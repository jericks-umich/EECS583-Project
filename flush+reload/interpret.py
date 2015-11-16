#!/usr/bin/env python3

THRESHOLD = 110

slices = [] # list of timeslices we're interested in
guesses = [] # list of bit guesses

class Guess:
  def __init__(self):
    self.s = False
    self.sr = False
    self.m = False
    self.mr = False
    self.last = None
    self.guess = -1
  def __repr__(self):
    return "guess: %d: "%self.guess + \
            ("s" if self.s else " ")+("r" if self.sr else " ")+ \
            ("m" if self.m else " ")+("r" if self.mr else " ")

class Timeslice:
  def __init__(self, s):
    self.sqr,self.mult,self.red = map(int,s.split(" "))
    self.s = self.sqr < THRESHOLD
    self.m = self.mult < THRESHOLD
    self.r = self.red < THRESHOLD
    self.smr = (4 * bool(self.s)) + (2 * bool(self.m)) + (1 * bool(self.r))
  def __repr__(self):
    return "(%d,%d,%d)"%(self.sqr,self.mult,self.red)
  def __str__(self):
    ret = ""
    ret += "s" if self.s else " "
    ret += "m" if self.m else " "
    ret += "r" if self.r else " "
    return ret

def load(fname):
  startindex = 0
  endindex = 0
  with open(fname,"r") as f:
    lines = [line.rstrip() for line in f] # yes, we read them all into memory, hope the file is small enough
  for i,line in enumerate(lines):
    if sum(map(lambda x:len(x)<3, line.split(" "))): # if any less than three characters
      startindex = i
      break
  for i,line in enumerate(reversed(lines)):
    if sum(map(lambda x:len(x)<3, line.split(" "))): # if any less than three characters
      endindex = len(lines)-i
      break
  print("startindex = %d, endindex = %d"%(startindex,endindex))
  for line in lines[startindex:endindex]:
    slices.append(Timeslice(line))
  return

def interpret():
  cg = Guess() # current guess
  for ts in slices:
    if ts == cg.last:
      continue # skip duplicates
    if ts.smr == 0:
      cg.last = ts
      continue # skip blanks after clearing last

    if ts.r:
      if cg.s:
        if cg.m:
          cg.mr = True
          cg.guess = 1
          guesses.append(cg)
          cg = Guess()
        else:
          cg.sr = True
      # if neither s nor m marked, discard r

    if ts.m:
      if cg.s: # only mark m if s is already marked
        cg.m = True

    if ts.s:
      if cg.s:
        if cg.m:
          cg.guess = 1
          guesses.append(cg)
          cg = Guess()
          continue # short circuit rest of s processing
        if cg.sr and not ts.r: # if we have sr from previous, but not this one
          cg.guess = 0
          guesses.append(cg)
          cg = Guess()
          continue # short circuit rest of s processing
        if cg.last and cg.last.smr == 0: # if last was blank
          # if we get here, it means we had an s, then a blank line, then another s
          cg.guess = 0
          guesses.append(cg) # so guess a 0 and then let it fall through to re-set s
          cg = Guess()
      cg.s = True

    cg.last = ts

def main():
  # load only time slices of interest
  load("output.txt")

  for s in slices:
    print(s)
  print(len(slices))

  interpret()

  print("".join([str(g.guess) for g in guesses]))
  print(len(guesses))
  for i in range(10):
    print(guesses[i])


if __name__ == "__main__":
  main()
