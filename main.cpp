// 25d000 цвет для лучей и игрока
// 37, 208, 0 в RGB

#include <cmath>
#include <vector>
#include <boost/geometry/geometry.hpp>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

typedef boost::geometry::model::d2::point_xy<int> Point;
typedef boost::geometry::model::box<boost::geometry::model::point<int, 2, boost::geometry::cs::cartesian>> Box;
typedef boost::geometry::model::segment<boost::geometry::model::point<int, 2, boost::geometry::cs::cartesian>> Segment;

bool isRunning = true;

class Screen
{
public:
    SDL_Event e;
    SDL_Window *window;
    SDL_Renderer *renderer;

    Screen()
    {
        SDL_Init(SDL_INIT_VIDEO);
        SDL_DisplayMode displayMode;
        SDL_GetCurrentDisplayMode(0, &displayMode);
        window = SDL_CreateWindow("3D game", SDL_WINDOWPOS_UNDEFINED,
                                  SDL_WINDOWPOS_UNDEFINED, displayMode.w,
                                  displayMode.h, SDL_WINDOW_FULLSCREEN);
        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    }
    ~Screen()
    {
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
    }
};

class Map
{
public:
    // при отображении без уменьшения эти прямоугольники частично за пределами экрана
    // игровая зона от 1, 1 до 1700, 1700
    // первая стена граница карты
    std::vector<SDL_Rect> walls = {
        {1, 1, 1700, 1700},
        {1250, 1300, 150, 200},
        {250, 300, 350, 400},
        {450, 500, 550, 600}};

    // указывать значение от 0 до 1700
    SDL_Rect player = {1240, 850, 30, 30};

    int playerAngle = 0;

    SDL_Surface *playerMarkerIMG = IMG_Load("player_marker.png");
    SDL_Texture *playerMarker;

    SDL_Renderer *renderer;
    Map(SDL_Renderer *renderer) : renderer(renderer)
    {
        playerMarker = SDL_CreateTextureFromSurface(renderer, playerMarkerIMG);
    }

    ~Map()
    {
        SDL_DestroyTexture(playerMarker);
    }

    int FixAng(int a)
    {
        if (a > 359)
        {
            a -= 360;
        }
        if (a < 0)
        {
            a += 360;
        }
        return a;
    }

    float degToRad(int a) { return a * M_PI / 180.0; }

    //! я остановился тут, надо чтобы лучи исходили из центра персонажа а не угла
    void drawRayAnd3D(int a)
    {
        int rayLength = 680; // Длина луча

        int endX = round((player.x) + cos(degToRad(playerAngle + a)) * rayLength);
        int endY = round((player.y) + sin(degToRad(playerAngle + a)) * rayLength);

        std::vector<Point> intersectionPoints;

        for (int i = 0; i < static_cast<int>(walls.size()); i++)
        {
            Segment topSegment = {Point(walls[i].x, walls[i].y), Point(walls[i].x + walls[i].w, walls[i].y)};
            Segment bottomSegment = {Point(walls[i].x, walls[i].y + walls[i].h), Point(walls[i].x + walls[i].w, walls[i].y + walls[i].h)};
            Segment leftSegment = {Point(walls[i].x, walls[i].y), Point(walls[i].x, walls[i].y + walls[i].h)};
            Segment rightSegment = {Point(walls[i].x + walls[i].w, walls[i].y), Point(walls[i].x + walls[i].w, walls[i].y + walls[i].h)};
            Segment raySegment = {Point(player.x, player.y), Point(endX, endY)};

            boost::geometry::intersection(raySegment, topSegment, intersectionPoints);
            boost::geometry::intersection(raySegment, bottomSegment, intersectionPoints);
            boost::geometry::intersection(raySegment, leftSegment, intersectionPoints);
            boost::geometry::intersection(raySegment, rightSegment, intersectionPoints);
        }

        Point closestPoint(0, 0);
        boost::geometry::index::rtree<Point, boost::geometry::index::quadratic<16>> rtree(intersectionPoints.begin(), intersectionPoints.end());
        rtree.query(boost::geometry::index::nearest(Point(player.x, player.y), 1), &closestPoint);

        if (boost::geometry::get<0>(closestPoint) != 0 && boost::geometry::get<1>(closestPoint) != 0)
        {
            endX = boost::geometry::get<0>(closestPoint);
            endY = boost::geometry::get<1>(closestPoint);
        }

        if (a == 30)
        {
            for (int i = 0; i < static_cast<int>(walls.size()); i++)
            {
                SDL_Rect wall = walls[i];
                wall.x /= 5;
                wall.y /= 5;
                wall.w /= 5;
                wall.h /= 5;

                SDL_SetRenderDrawColor(renderer, 220, 220, 220, 255);
                SDL_RenderDrawRect(renderer, &wall);
            }
        }

        if (endX != round((player.x) + cos(degToRad(playerAngle + a)) * rayLength) and endY != round((player.y) + sin(degToRad(playerAngle + a)) * rayLength))
        {
            int distanceToClosestPoint = round(boost::geometry::distance(Point(player.x, player.y), Point(endX, endY)));
            SDL_Rect line3D = {a + 120 * 6, distanceToClosestPoint / 2, 10, rayLength - (distanceToClosestPoint / 2) * 2}; //! чинить, для починки увеличить изображение и ничего более
            SDL_SetRenderDrawColor(renderer, rayLength - distanceToClosestPoint / 5, rayLength - distanceToClosestPoint / 5, rayLength - distanceToClosestPoint / 5, 255);
            SDL_RenderFillRect(renderer, &line3D);
        }

        // кривое отображение лучей для тестов
        SDL_SetRenderDrawColor(renderer, 37, 208, 0, 255);
        SDL_RenderDrawLine(renderer, (player.x / 5), (player.y / 5), (endX / 5), (endY / 5));
    }

    void display() // прямоугольники уменьшены в 5 раз на мини карте в сравнении с вектором walls
    {
        for (int i = -30; i <= 30; i++)
        {
            drawRayAnd3D(i);
        }

        SDL_Rect minimapPlayer = {player.x / 5 - 15, player.y / 5 - 15, player.w, player.h};
        SDL_RenderCopy(renderer, playerMarker, NULL, &minimapPlayer);
    }
};

int main(int argc, char **args)
{
    Screen screen;
    Map map(screen.renderer);

    bool keyStates[4] = {false, false, false, false}; // W, S, A, D

    while (isRunning)
    {
        while (SDL_PollEvent(&screen.e) != 0)
        {
            if (screen.e.type == SDL_QUIT)
            {
                isRunning = false;
            }
            else if (screen.e.type == SDL_KEYDOWN || screen.e.type == SDL_KEYUP)
            {
                bool keyDown = (screen.e.type == SDL_KEYDOWN);
                if (screen.e.key.keysym.scancode == SDL_SCANCODE_W) // W key
                    keyStates[0] = keyDown;
                if (screen.e.key.keysym.scancode == SDL_SCANCODE_S) // S key
                    keyStates[1] = keyDown;
                if (screen.e.key.keysym.scancode == SDL_SCANCODE_A) // Q key
                    keyStates[2] = keyDown;
                if (screen.e.key.keysym.scancode == SDL_SCANCODE_D) // D key
                    keyStates[3] = keyDown;
                if (screen.e.key.keysym.scancode == SDL_SCANCODE_ESCAPE)
                    isRunning = false;
            }
        }

        if (keyStates[0]) // W key
        {
            map.player.x += round(cos(map.degToRad(map.playerAngle)) * 4);
            map.player.y += round(sin(map.degToRad(map.playerAngle)) * 4);
        }
        if (keyStates[1]) // S key
        {
            map.player.x -= round(cos(map.degToRad(map.playerAngle)) * 4);
            map.player.y -= round(sin(map.degToRad(map.playerAngle)) * 4);
        }
        if (keyStates[2]) // A key
        {
            map.playerAngle -= 5;
        }
        if (keyStates[3]) // D key
        {
            map.playerAngle += 5;
        }

        map.playerAngle = map.FixAng(map.playerAngle);

        SDL_SetRenderDrawColor(screen.renderer, 0, 0, 0, 255);
        SDL_RenderClear(screen.renderer);

        map.display();

        SDL_RenderPresent(screen.renderer);
        SDL_Delay(16);
    }

    SDL_Quit();
    return 0;
}