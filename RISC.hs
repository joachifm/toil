{-# LANGUAGE OverloadedStrings #-}

--
-- A port of the RISC emulator described in
-- N. Wirth, Compiler Construction

module RISC (
  CPU,
  cpu,
  step,
  execute,
  executeSpec,
  ) where

import qualified Test.Hspec as Spec

import Data.Int
import Data.Word
import Data.Map (Map)
import qualified Data.Map.Strict as Map
import Data.Bits

type Adr = Word32

type Off = Adr

type Imm = Int32

data Reg
  = R0 | R1 | R2 | R3 | R4 | R5 | R6 | R7 | R8 | R9 | R10 | R11 | R12
  | R13 | R14 | R15
  deriving (Eq, Ord, Enum, Show, Read)

data Arg = R !Reg | I !Imm
  deriving (Eq, Ord, Show, Read)

data BranchCond
  = IfNeg       -- N
  | IfPos       -- ~N
  | IfZero      -- Z
  | IfNotZero   -- ~Z
  | IfCarry     -- Carry (C) set
  | IfOverflow  -- Overflow (V) set
  | IfLess      -- N /= V
  | IfLessEq    -- (N /= V)|Z
  | IfGreater   -- ~((N /= V)|Z)
  | Never
  | Always
  deriving (Eq, Show, Read)

data Instr
  = Mov Reg Arg       -- MOV a,n      R.a := n
  | Add Reg Reg Arg   -- ADD a,b,n    R.a := R.b + n
  | Sub Reg Reg Arg   -- SUB a,b,n    R.a := R.b - n
  | Mul Reg Reg Arg   -- MUL a,b,n    R.a := R.b * n
  | Div Reg Reg Arg   -- DIV a,b,n    R.a := R.b / n
  | And Reg Reg Arg   -- AND a,b,n    R.a := R.b & n
  | Load Reg Reg Off  -- LD a,b,off   R.a := M[R.b+off]
  | Store Reg Reg Off -- ST a,b,off   M[R.b+off] := R.a
  | Branch
    { branchSaveLink :: Bool -- if true, R[15] := PC+1
    , branchDestDest :: Arg      -- if reg, PC := R.c else PC := PC+1+off
    , branchCond :: BranchCond
    }
  | Nop
  | Hlt
  deriving (Eq, Show, Read)

data Cell = Code Instr | Data Imm
  deriving (Eq, Show, Read)

data CPU = CPU
  { pc :: Adr
  , ir :: Adr
  , rs :: Map Reg Imm
  , mm :: Map Adr Cell
  } deriving (Eq, Show, Read)

regMap :: Map Reg Imm
regMap = Map.fromList (zip [R0 ..] (repeat 0))

memMap :: Int -> Map Adr Cell
memMap len = Map.fromList (zip [0..fromIntegral len] (repeat (Data 0)))

cpu :: CPU
cpu = CPU
  { pc = 0
  , ir = 0
  , rs = regMap
  , mm = memMap memLen
  }

readReg :: Reg -> CPU -> Imm
readReg r st = case Map.lookup r (rs st) of
  Just v -> v
  Nothing -> error ("readReg: CPU misses register " ++ show r)

writeReg :: Reg -> Imm -> CPU -> CPU
writeReg r v st = st { rs = Map.insert r v (rs st) }

readMem :: Adr -> CPU -> Cell
readMem r st = case Map.lookup r (mm st) of
  Just v -> v
  Nothing -> error ("readMem: invalid memory address " ++ show r)

writeMem :: Adr -> Cell -> CPU -> CPU
writeMem r v st = st { mm = Map.insert r v (mm st) }

memLen :: Int
memLen = 64

step :: CPU -> CPU
step st =
  case Map.lookup (pc st) (mm st) of
    Nothing -> error "invalid address"
    Just (Code instr) -> execute instr (st { pc = pc st + 1, ir = pc st })
    Just _ -> error "invalid code"

execute :: Instr -> CPU -> CPU
execute instr st =
  case instr of
    -- MOV
    Mov a (I n) ->
      st { rs = Map.insert a n (rs st) }

    Mov a (R b) ->
      st { rs = Map.insert a (readReg b st) (rs st) }

    -- ADD
    Add a b (I n) ->
      st { rs = Map.insert a (readReg b st + n) (rs st) }

    Add a b (R c) ->
      st { rs = Map.insert a (readReg b st + readReg c st) (rs st) }

    -- MUL
    Mul a b (I n) ->
      st { rs = Map.insert a (readReg b st * n) (rs st) }

    Mul a b (R c) ->
      st { rs = Map.insert a (readReg b st * readReg c st) (rs st) }

    -- AND
    And a b (I n) ->
      st { rs = Map.insert a (readReg b st .&. n) (rs st) }

    And a b (R c) ->
      st { rs = Map.insert a (readReg b st .&. readReg c st) (rs st) }

    -- LD
    Load a b off ->
      let Data v = readMem (fromIntegral (readReg b st) + off) st
      in writeReg a v st

    -- ST
    Store a b off ->
      writeMem (fromIntegral (readReg b st) + off)
               (Data (readReg a st))
               st

executeSpec :: Spec.Spec
executeSpec = Spec.describe "execute" $ do
  Spec.it "mov with immediate value" $ do
    execute (Mov R0 (I 42)) cpu
    `Spec.shouldBe` cpu { rs = Map.insert R0 42 (rs cpu) }

  Spec.it "mov with register" $ do
    execute (Mov R1 (R R0)) (execute (Mov R0 (I 42)) cpu)
    `Spec.shouldSatisfy` (\st -> readReg R1 st == 42)

  Spec.it "store without offset" $ do
    execute (Store R0 R1 0)
            (execute (Mov R0 (I 42))
                     (execute (Mov R1 (I 0)) cpu))
    `Spec.shouldSatisfy` (\st -> readMem 0 st == Data 42)
