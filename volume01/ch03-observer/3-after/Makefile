# 本書の掲載コードをそのままビルドして動かすための Makefile です。
#   make         ビルド
#   make run     ビルドして実行
#   make verify  実行結果が expected.txt（本に載っている実行結果）と一致するか検査
#   make clean   生成物を消す

CXX      ?= g++
CXXFLAGS ?= -std=c++14 -Wall -I.
TARGET   := app
SRCS     := $(wildcard *.cpp)

$(TARGET): $(SRCS) $(wildcard *.h)
	$(CXX) $(CXXFLAGS) $(SRCS) -o $@

run: $(TARGET)
	./$(TARGET)

verify: $(TARGET)
	@./$(TARGET) > actual.txt
	@diff -u expected.txt actual.txt && echo "OK: 本の実行結果と一致しました"

clean:
	rm -f $(TARGET) actual.txt

.PHONY: run verify clean
