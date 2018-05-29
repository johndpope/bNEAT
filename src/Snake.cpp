//
// Created by Sheldon Woodward on 5/12/18.
//

#include <algorithm>
#include <iostream>
#include <fstream>
#include "Snake.hpp"

Snake::Snake(int sizeX, int sizeY) {
    timeOut = sizeX * sizeY;
    width = sizeX;
    height = sizeY;
}

int Snake::getWidth() {
    return width;
}

int Snake::getHeight() {
    return height;
}

int Snake::fitness(ANN &agent, bool record) {
    snake = std::deque<std::pair<int, int>>();
    snake.push_front(std::pair(width / 2, height / 2));
    snake.push_front(std::pair(width / 2, (height / 2) - 1));
    std::deque<float> input(width * height, 0);
    std::deque<float> output;
    food = std::nullopt;
    int time = -1;

    // record begin
    if (record) {
        std::ofstream logFile;
        logFile.open("logFile.txt", std::ios::app);
        logFile << "Board: " << width << "," << height << std::endl;
        logFile.close();
    }

    do {
        time += 1;
        if (!food) {
            generateFood();
        }

//        std::cout << "food: " << food.value().first << ", " << food.value().second << std::endl;
        for (auto it : snake) {
//            std::cout << "snake: " << it.first << ", " << it.second << std::endl;
        }

        // record body
        if (record) {
            std::ofstream logFile;
            logFile.open("logFile.txt", std::ios::app);
            logFile << food.value().first << "," << food.value().second << std::endl;
            for (auto it : snake) {
                logFile << it.first << "," << it.second << std::endl;
            }
            logFile << "@@@" << std::endl;
            logFile.close();
        }

        parseInput(input);

        output = agent.compute(input);
//        for (auto it : output) {
//            std::cout << it << std::endl;
//        }

//        std::cout << std::endl;
        switch (validMove(output)) {
            case 0:
                snake.push_front(std::pair(snake[0].first - 1, snake[0].second));
                break;
            case 1:
                snake.push_front(std::pair(snake[0].first + 1, snake[0].second));
                break;
            case 2:;
                snake.push_front(std::pair(snake[0].first, snake[0].second + 1));
                break;
            case 3:;
                snake.push_front(std::pair(snake[0].first, snake[0].second - 1));
                break;
        }

        // Did the snake eat
        if (snake.front() == food) {
            food = std::nullopt;
        } else {
            snake.pop_back();
        }


    } while (!gameOver(time));
    //Log end
    if (record) {
        std::ofstream logFile;
        logFile.open("logFile.txt", std::ios::app);
        logFile << "###\n";
        logFile.close();
    }

    return snake.size() - 2;
}


bool Snake::gameOver(int time) {
    if (time == timeOut) {
        return true;
    }
    if (snake.front().first > width || snake.front().first < 0 || snake.front().second > height ||
        snake.front().second < 0) {
        return true;
    }
    for (int i = 1; i < snake.size(); i++) {
        if (snake[0] == snake[i]) {
            return true;
        }
    }

    return false;
}

int Snake::validMove(std::deque<float> &output) {
    int max = 0;
    int max2 = 0;

    for (int i = 0; i < output.size(); i++) {
        if (output[i] >= output[max]) {
            max2 = max;
            max = i;
        }
    }

    switch (max) {
        case 0: {
            if (snake[0].first - 1 == snake[1].first && snake[0].second == snake[1].second) {
                return max2;
            };
            break;
        }
        case 1: {
            if (snake[0].first + 1 == snake[1].first && snake[0].second == snake[1].second) {
                return max2;
            };
            break;
        }
        case 2: {
            if (snake[0].first == snake[1].first && snake[0].second + 1 == snake[1].second) {
                return max2;
            };
            break;
        }
        case 3: {
            if (snake[0].first - 1 == snake[1].first && snake[0].second - 1 == snake[1].second) {
                return max2;
            };
            break;
        }
    }

    return max;
}

void Snake::parseInput(std::deque<float> &input) {
    // parse snake into input
    for (int i = 0; i < input.size(); i++) {
        if (find(snake.begin(), snake.end(), std::pair(i % width, i / width)) != snake.end()) {
            input[i] = -1;
        } else {
            input[i] = 0;
        }
    }

    // parse food into input
    if (food) {
        input[width * food->first + food->first] = 1;
    }
}

void Snake::generateFood() {
    std::pair<int, int> testFood;
    do {
        testFood.first = rand() % width;
        testFood.second = rand() % height;
    } while (find(snake.begin(), snake.end(), testFood) != snake.end());

    food = testFood;
}
