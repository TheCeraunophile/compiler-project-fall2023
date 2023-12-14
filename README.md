# Compiler project (Fall 2023)

A simple compiler project based on llvm. You can find the report [here](https://drive.google.com/file/d/1-TXCX1-H59gi1oofKAx05_4pMWJ_XzKL/view?usp=sharing)

## How to run?
```
mkdir build
cd build
cmake ..
make
cd src
./gsm "your code" > gsm.ll
llc --filetype=obj -o=gsm.o gsm.ll
clang -o gsmbin gsm.o ../../rtGSM.c
```

## Decleration syntax
```
int a, b, c, d = 1, 5, 10;   // a=1, b=5, c=10, d=0
```

## Assignment syntax
```
a = 3 * 9;
a += 12;
```

## Loop syntax
```
loopc c >= a:
begin
    a += 1;
    ...
end
```

## If, elif and else syntax
```
if a>b:
begin
    ...
end
elif a<b:
begin
    ...
end
else
begin
    ...
end
