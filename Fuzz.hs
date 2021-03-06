{-|
A simple-minded fuzzer input generator.

Prints syntactically (but not necessarily semantically) valid programs to
stdout.
-}

module Main (main) where

import Control.Monad (replicateM_)
import qualified System.Random as Rand

oneof :: [IO a] -> IO a
oneof xs = do
  i <- Rand.randomRIO (0, length xs - 1)
  xs !! i

-- Define a generator for each language construct

parenExpr = putStr "(" >> expression >> putStr ")"

constant = oneof (map (putStr . show) [0..512])

variable = oneof (map (\c -> putStr [c]) ['a'..'z'])

factor = oneof [constant, variable, parenExpr]

term = do
  factor
  oneof [putStr "*", putStr "/"]
  factor

arithExpression = do
  term
  oneof [putStr "+", putStr "-"]
  term

relationExpression = do
  arithExpression
  oneof [putStr ">", putStr "<", putStr "="]
  arithExpression

booleanExpression = do
  relationExpression
  putStr " "
  oneof [putStr "AND", putStr "OR"]
  putStr " "
  relationExpression

expression = oneof [factor, booleanExpression]

callStmt = variable >> putStr "()" >> putStrLn ""

assignStmt =
  variable >> putStr " " >> putStr ":=" >> putStr " " >> expression >> putStrLn ""

whileStmt = do
  putStr "WHILE" >> putStr " " >> expression >> putStrLn ""
  blockStmt
  putStrLn ""
  putStrLn "ENDWHILE"

ifStmt = do
  putStr "IF" >> putStr " " >> expression >> putStrLn ""
  blockStmt
  putStrLn "ELSE"
  blockStmt
  putStrLn "ENDIF"

blockStmt = oneof [ifStmt, assignStmt, callStmt]

varDeclStmt = do
  putStr "VAR" >> putStr " " >> variable >> putStr " " >> putStrLn "INT"

procDeclStmt = do
  putStr "PROC" >> putStr " " >> variable >> putStrLn ""
  blockStmt
  putStrLn "ENDPROC"

programStmt = do
  putStr "PROGRAM" >> putStr " " >> variable >> putStrLn ""
  varDeclStmt
  procDeclStmt
  blockStmt
  putStrLn "END"

-- Do it.
main :: IO ()
main = programStmt
