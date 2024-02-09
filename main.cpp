#include <iostream>
#include <SFML/Graphics.hpp>
#include "json.hpp"
#include<fstream>
#include<set>
#include<cmath>
#include "CppSockets/sockets/TcpServerSocket.hpp"
#include<future>
#include<thread>
#include<string>
#include<sstream>
#include <chrono>

#define NOGUI false
#define screenScale 300

std::chrono::microseconds pms = duration_cast<std::chrono::microseconds>(
        std::chrono::system_clock::now().time_since_epoch()
);

int get_sign(double x) {
    if (x > 0)
        return 1;
    if (x < 0)
        return -1;
    return 1;
}

struct Dot {
    double x, y;

    Dot(double _x = 0, double _y = 0) : x(_x), y(_y) {};

    Dot getCoord(double mx, double my, double a) {
        return Dot(mx + x * cos(-a) - y * sin(-a),
                   my + x * sin(-a) + y * cos(-a));
    }
};


struct Robot {
    double x = 0.1, y = 0.1, px = 0, py = 0, angle = 0, headSize = 0.07;
    std::vector<Dot> dots = {{0.095,  0.055},
                             {0.095,  -0.055},
                             {-0.095, -0.055},
                             {-0.095, 0.055}};
    Dot centralDot{0, 0.055};
    sf::Color color = sf::Color::White;

    Robot() {};

    void rotate(double a) {
        angle += a;
        if (angle >= 2 * M_PI)
            angle = angle - 2 * M_PI;
        else if (angle <= 0) {
            angle = M_PI * 2 + angle;
        }
        //std::cout << angle << std::endl;
    }

    void move(double X, double Y, double maxX, double maxY) {
        px = x;
        py = y;
        x += X * cos(angle) + Y * sin(angle);
        y += -X * sin(angle) + Y * cos(angle);
        if (x < 0)
            x = 0;
        if (y < 0)
            y = 0;
        if (x > maxX + 1.2)
            x = maxX + 1.2;
        if (y > maxY + 1.2)
            y = maxY + 1.2;
    }

    void socket(int z) {

        TcpServerSocket server("localhost", z);
        server.acceptConnection();
        std::cout << z << "\n";
        std::cout << "OK\n";
        char o[100];
        while (true) {
            server.receiveData(o, 100);
            if (o[0] == 'q')
                break;
            std::cout << o << "\n";
            std::stringstream s(o);
            double _x, _y, _a;
            s >> _x >> _y >> _a;
            x += _x;
            y += _y;
            angle += _a;
        }

    }


};

struct Line {
    double x1, y1, x2, y2;

    Line(double X1, double Y1, double X2, double Y2) {
        x1 = std::min(X1, X2);
        x2 = std::max(X1, X2);
        y1 = std::min(Y1, Y2);
        y2 = std::max(Y1, Y2);
    }

    bool operator<(const Line l) const {
        int sx1 = (x1 + x2) / 2;
        int sx2 = (l.x1 + l.x2) / 2;
        int sy1 = (y1 + y2) / 2;
        int sy2 = (l.y1 + l.y2) / 2;
        if (sx1 == sx2)
            return sy1 < sy2;
        return sx1 < sx2;
    }

    bool operator>(const Line l) const {
        int sx1 = (x1 + x2) / 2;
        int sx2 = (l.x1 + l.x2) / 2;
        int sy1 = (y1 + y2) / 2;
        int sy2 = (l.y1 + l.y2) / 2;
        if (sx1 == sx2)
            return sy1 > sy2;
        return sx1 > sx2;
    }

    bool operator==(const Line l) const {
        int sx1 = (x1 + x2) / 2;
        int sx2 = (l.x1 + l.x2) / 2;
        int sy1 = (y1 + y2) / 2;
        int sy2 = (l.y1 + l.y2) / 2;
        return sx1 == sx2 && sy1 == sy2;
    }

    bool operator!=(const Line l) const {
        int sx1 = (x1 + x2) / 2;
        int sx2 = (l.x1 + l.x2) / 2;
        int sy1 = (y1 + y2) / 2;
        int sy2 = (l.y1 + l.y2) / 2;
        return !(sx1 == sx2 && sy1 == sy2);
    }

    friend std::ostream &operator<<(std::ostream &o, Line &l) {
        o << l.x1 << " " << l.y1 << "|" << l.x2 << " " << l.y2 << std::endl;
        return o;
    }
};

struct Ball {
    double x, y, ax, ay, sx, sy, s = 0.08;
    sf::Color color;

    Ball(double _x, double _y) : x(_x), y(_y) {}

    void tick() {
        bool check = false;
        x += ax;
        y += ay;
        if (ax < 0)
            ax += std::min(abs(ax), 0.0001);
        else
            ax -= std::min(abs(ax), 0.0001);
        if (ay < 0)
            ay += std::min(abs(ay), 0.0001);
        else
            ay -= std::min(abs(ay), 0.0001);
        /*
        if (check)
            std::cout << "\n";
            */
    }

    void check_robot(Robot &robot, int size) {
        if (sqrt(pow(robot.x - x, 2) + pow(robot.y - y, 2)) <= robot.headSize + s) {
            ax += (robot.x - robot.px);
            ay += (robot.y - robot.py);
            //std::cout<<"YES|"<<ax<<"|"<<ay<<"\n";
        }
    }

    void checkRobotPush(Robot &robot, int size) {
        Dot c = robot.centralDot.getCoord(robot.x, robot.y, robot.angle);
        if (sqrt(pow(c.x - x, 2) + pow(c.y - y, 2)) <= robot.headSize + s + 0.01) {
            ax += 0.01 * cos(robot.angle - M_PI / 4) + 0.01 * sin(robot.angle - M_PI / 4);
            ay += -0.01 * sin(robot.angle - M_PI / 4) + 0.01 * cos(robot.angle - M_PI / 4);
        }
    }

    void check_borders(int xSize, int ySize, int scale) {
        if (x >= (double) xSize / scale + 0.1) {
            x = (double) xSize / scale / 2 + (double) 60 / scale;
            ax = 0;
            y = (double) ySize / scale / 2 + (double) 60 / scale;
            ay = 0;
            std::cout << "border 1\n";
        }
        if (y >= (double) ySize / scale + 0.1) {
            x = (double) xSize / scale / 2 + (double) 60 / scale;
            ax = 0;
            y = (double) ySize / scale / 2 + (double) 60 / scale;
            ay = 0;
            std::cout << "border 2\n";
        }
        if (x <= 0.1) {
            x = (double) xSize / scale / 2 + (double) 60 / scale;
            ax = 0;
            y = (double) ySize / scale / 2 + (double) 60 / scale;
            ay = 0;
            std::cout << "border 3\n";
        }
        if (y <= 0.1) {
            x = (double) xSize / scale / 2 + (double) 60 / scale;
            ax = 0;
            y = (double) ySize / scale / 2 + (double) 60 / scale;
            ay = 0;
            std::cout << "border 4 \n";
        }
    }

};

struct Field {
    nlohmann::json json;
    int xSize;
    int ySize;
    int scale;
    int indent = 60;

    Field(std::string fileName, int s = 500) {
        std::ifstream fin(fileName);
        std::string text;
        std::string c;
        scale = s;
        if (!fin.is_open()) {
            std::cout << "JSON file name is invalid.\n";
            abort();
        }
        while (!fin.eof()) {
            getline(fin, c);
            text += c;
        }
        json = nlohmann::json::parse(text);
        fin.close();
        std::cout << json << std::endl;
        xSize = (int) (((float) json["FIELD_LENGTH"]) * (float) scale);
        ySize = (int) (((float) json["FIELD_WIDTH"]) * (float) scale);
    }

    [[nodiscard]] sf::VideoMode getWindowSize() const {
        return sf::VideoMode(xSize + indent * 2, ySize + indent * 2);
    }

    void drawField(sf::RenderWindow &window) {

        sf::RectangleShape backgroundColor;
        backgroundColor.setSize(sf::Vector2f(xSize + indent * 2, ySize + indent * 2));
        backgroundColor.setPosition(0, 0);
        backgroundColor.setFillColor(sf::Color::Green);
        window.draw(backgroundColor);

        sf::RectangleShape baseRect;
        baseRect.setSize(sf::Vector2f(xSize, ySize));
        baseRect.setPosition(indent, indent);
        baseRect.setFillColor(sf::Color::Green);
        baseRect.setOutlineColor(sf::Color::White);
        baseRect.setOutlineThickness(0.05 * scale);
        window.draw(baseRect);

        sf::CircleShape middleCircle;
        middleCircle.setRadius(0.5 * scale);
        middleCircle.setPosition(xSize / 2 + indent - 0.5 * scale, ySize / 2 + indent - 0.5 * scale);
        middleCircle.setFillColor(sf::Color::Green);
        middleCircle.setOutlineColor(sf::Color::White);
        middleCircle.setOutlineThickness(0.05 * scale);
        window.draw(middleCircle);

        sf::RectangleShape middleLine;
        middleLine.setSize(sf::Vector2f(0.05 * scale, ySize));
        middleLine.setPosition(xSize / 2 + indent - 0.025 * scale, indent);
        middleLine.setFillColor(sf::Color::White);
        middleLine.setOutlineColor(sf::Color::White);
        middleLine.setOutlineThickness(0);
        window.draw(middleLine);

        sf::CircleShape leftRedGoalCircle;
        leftRedGoalCircle.setRadius(10);
        leftRedGoalCircle.setPosition(xSize + indent - 10, ySize / 2 + indent - 200 - 10);
        leftRedGoalCircle.setFillColor(sf::Color::Red);
        leftRedGoalCircle.setOutlineColor(sf::Color::White);
        leftRedGoalCircle.setOutlineThickness(0);
        window.draw(leftRedGoalCircle);

        sf::CircleShape rightRedGoalCircle;
        rightRedGoalCircle.setRadius(10);
        rightRedGoalCircle.setPosition(xSize + indent - 10, ySize / 2 + indent + 200 - 10);
        rightRedGoalCircle.setFillColor(sf::Color::Red);
        rightRedGoalCircle.setOutlineColor(sf::Color::White);
        rightRedGoalCircle.setOutlineThickness(0);
        window.draw(rightRedGoalCircle);

        sf::CircleShape leftBlueGoalCircle;
        leftBlueGoalCircle.setRadius(10);
        leftBlueGoalCircle.setPosition(indent - 10, ySize / 2 + indent - 200 - 10);
        leftBlueGoalCircle.setFillColor(sf::Color::Blue);
        leftBlueGoalCircle.setOutlineColor(sf::Color::White);
        leftBlueGoalCircle.setOutlineThickness(0);
        window.draw(leftBlueGoalCircle);

        sf::CircleShape rightBlueGoalCircle;
        rightBlueGoalCircle.setRadius(10);
        rightBlueGoalCircle.setPosition(indent - 10, ySize / 2 + indent + 200 - 10);
        rightBlueGoalCircle.setFillColor(sf::Color::Blue);
        rightBlueGoalCircle.setOutlineColor(sf::Color::White);
        rightBlueGoalCircle.setOutlineThickness(0);
        window.draw(rightBlueGoalCircle);

    }

    void drawRobot(Robot &robot, sf::RenderWindow &window) {
        sf::CircleShape R;
        R.setRadius(12);
        Dot c = robot.centralDot.getCoord(robot.x, robot.y, robot.angle);
        R.setPosition(c.x * scale - 12, c.y * scale - 12);
        R.setFillColor(sf::Color::Magenta);
        R.setOutlineColor(robot.color);
        window.draw(R);
        R.setRadius(robot.headSize * scale);
        R.setPosition(robot.x * scale - robot.headSize * scale, robot.y * scale - robot.headSize * scale);
        R.setFillColor(robot.color);
        window.draw(R);
        sf::VertexArray line(sf::Lines, 2);
        line[0] = sf::Vector2f(robot.x * scale, robot.y * scale);
        line[1] = sf::Vector2f(robot.x * scale + 50 * sin(robot.angle),
                               robot.y * scale + 50 * cos(robot.angle));
        line[0].color = robot.color;
        line[1].color = sf::Color::Red;
        window.draw(line);
        for (int i = 0; i < 3; ++i) {
            Dot x = robot.dots[i].getCoord(robot.x, robot.y, robot.angle);
            Dot x1 = robot.dots[i + 1].getCoord(robot.x, robot.y, robot.angle);
            //std::cout << i << "|" << i + 1 << "\n";

            sf::CircleShape R;
            R.setRadius(6);
            R.setPosition(x.x * scale - 6, x.y * scale - 6);
            R.setFillColor(sf::Color::Magenta);
            //window.draw(R);
            sf::VertexArray line(sf::Lines, 2);
            line[0] = sf::Vector2f(x.x * scale, x.y * scale);
            line[1] = sf::Vector2f(x1.x * scale, x1.y * scale);
            line[0].color = robot.color;
            line[1].color = sf::Color::Red;
            window.draw(line);
        }
        Dot x = robot.dots[3].getCoord(robot.x, robot.y, robot.angle);
        Dot x1 = robot.dots[0].getCoord(robot.x, robot.y, robot.angle);
        line[0] = sf::Vector2f(x.x * scale, x.y * scale);
        line[1] = sf::Vector2f(x1.x * scale, x1.y * scale);
        line[0].color = robot.color;
        line[1].color = sf::Color::Red;
        window.draw(line);


    }

    void drawBall(Ball &ball, sf::RenderWindow &window) {
        sf::CircleShape R;
        R.setRadius(ball.s * scale);
        R.setPosition(ball.x * scale - ball.s * scale, ball.y * scale - ball.s * scale);
        R.setFillColor(ball.color);
        window.draw(R);
    }

};

void Socket(Robot &robot, int z) {
    robot.socket(z);
}

int main() {
    Field field("../file.json");
    sf::RenderWindow window(field.getWindowSize(), "Robofootbal ");

    Robot robot;
    robot.color = sf::Color::Red;
    Robot robot1;
    robot1.color = sf::Color::Blue;
    //std::thread t(Socket, std::ref(robot), 5040);

    Ball ball(0.5, 0.5);
    //ball.ax = 0.1;
    //sball.ay = 0.1;
    ball.color = sf::Color::Magenta;

    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed)
                window.close();

            else if (event.type == sf::Event::KeyPressed) {
                if (event.key.code == sf::Keyboard::S) {
                    robot.move(0, -0.01, field.xSize / field.scale, field.ySize / field.scale);
                }
                if (event.key.code == sf::Keyboard::W) {
                    robot.move(0, 0.01, field.xSize / field.scale, field.ySize / field.scale);
                }
                if (event.key.code == sf::Keyboard::D) {
                    robot.move(-0.01, 0, field.xSize / field.scale, field.ySize / field.scale);
                }
                if (event.key.code == sf::Keyboard::A) {
                    robot.move(0.01, 0, field.xSize / field.scale, field.ySize / field.scale);
                }
                if (event.key.code == sf::Keyboard::Right) {
                    robot.rotate(-0.05);
                }
                if (event.key.code == sf::Keyboard::Left) {
                    robot.rotate(0.05);
                }
                if (event.key.code == sf::Keyboard::Space) {
                    ball.checkRobotPush(robot, field.scale);
                }

            }

        }

        field.drawField(window);
        field.drawRobot(robot, window);
        field.drawRobot(robot1, window);
        ball.tick();
        ball.check_robot(robot, field.scale);
        field.drawBall(ball, window);
        ball.check_borders(field.xSize, field.ySize, field.scale);
        window.display();

        std::chrono::microseconds ms = duration_cast<std::chrono::microseconds>(
                std::chrono::system_clock::now().time_since_epoch()
        );

        while ((ms - pms).count() <= 1000000 / 60) {
            ms = duration_cast<std::chrono::microseconds>(
                    std::chrono::system_clock::now().time_since_epoch());
        }
        std::cout << (ms - pms).count()<<"\n";
        pms = duration_cast<std::chrono::microseconds>(
                std::chrono::system_clock::now().time_since_epoch()
        );

    }
}
