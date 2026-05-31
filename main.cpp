#include <bits/stdc++.h>
#include <SFML/Graphics.hpp>

using namespace std;

const int   ROWS    = 20;
const int   COLS    = 20;
const float CELL    = 28.f;
const float GAP     = 2.f;
const float PANEL_W = 170.f;
const float GRID_W  = COLS * CELL;
const float GRID_H  = ROWS * CELL;
const float WIN_W   = GRID_W + PANEL_W;
const float WIN_H   = GRID_H;

const sf::Color BFS_WAVE[8] = {
    {100,220,255},{75,195,248},{55,165,235},
    {45,135,215},{40,108,195},{35,84,172},
    {30,62,148},{25,44,122},
};

const sf::Color DFS_WAVE[8] = {
    {255,190, 80},{245,160, 55},{235,130, 35},
    {220,100, 20},{200, 75, 15},{175, 55, 10},
    {145, 38,  8},{115, 25,  5},
};

const sf::Color C_EMPTY {20, 20, 26};
const sf::Color C_WALL  {52, 52, 66};
const sf::Color C_START {46,204,113};
const sf::Color C_END   {231,76, 60};
const sf::Color C_PATH  {255,195, 30};

const sf::Color C_PANEL  {14, 14, 20};
const sf::Color C_DIVIDE {38, 38, 52};
const sf::Color C_LABEL  {100,100,130};
const sf::Color C_STATUS {145,145,175};

int  grid   [ROWS][COLS] = {};
int  waveAge[ROWS][COLS] = {};

pair<int,int> par[ROWS][COLS];   
bool          vis[ROWS][COLS] = {};

bool  algoRunning = false;
bool  tracing     = false;
bool  lastWasDFS  = false;
int   pathR = -1, pathC = -1;
int   waveStep    = 0;

int   cellsVisited = 0;
int   pathLen      = 0;
sf::Clock algoClock;

const float ALGO_DT = 0.016f;   
const float PATH_DT = 0.028f;

queue<pair<int,int>> bfsQ;
bool bfsRunning = false;

stack<pair<int,int>> dfsStk;
bool dfsRunning = false;


int tool   = 0;
int startR = -1, startC = -1;
int endR   = -1, endC   = -1;


void clearVis()
{
    for (int r = 0; r < ROWS; r++)
        for (int c = 0; c < COLS; c++)
            if (grid[r][c] == 4 || grid[r][c] == 5)
                grid[r][c] = 0;
    memset(waveAge, 0, sizeof(waveAge));
    waveStep = 0; cellsVisited = 0; pathLen = 0;
}

void clearAll()
{
    memset(grid,    0, sizeof(grid));
    memset(waveAge, 0, sizeof(waveAge));
    waveStep  = 0; cellsVisited = 0; pathLen = 0;
    startR = startC = endR = endC = -1;
    bfsRunning = dfsRunning = algoRunning = tracing = false;
    bfsQ  = {};
    dfsStk = {};
}

void stopAll()
{
    bfsRunning = dfsRunning = algoRunning = tracing = false;
    bfsQ  = {};
    dfsStk = {};
}


struct Button
{
    sf::RectangleShape   bg;
    unique_ptr<sf::Text> lbl;
    sf::CircleShape      dot;
    bool     hasDot = false;
    sf::Color base;

    void init(sf::Font& font, const string& text,
              float x, float y, float w, float h,
              sf::Color col,
              sf::Color dotCol = sf::Color::Transparent)
    {
        base = col;
        bg.setSize({w, h});
        bg.setPosition({x, y});
        bg.setFillColor({(uint8_t)(col.r/3),
                         (uint8_t)(col.g/3),
                         (uint8_t)(col.b/3), 255});
        bg.setOutlineThickness(0.f);
        bg.setOutlineColor(col);

        lbl = make_unique<sf::Text>(font, text, 12);
        lbl->setFillColor({210,210,225});
        auto b = lbl->getLocalBounds();
        lbl->setOrigin({b.position.x + b.size.x * .5f,
                        b.position.y + b.size.y * .5f});
        float offsetX = (dotCol != sf::Color::Transparent) ? 6.f : 0.f;
        lbl->setPosition({x + w * .5f + offsetX, y + h * .5f});

        hasDot = (dotCol != sf::Color::Transparent);
        if (hasDot)
        {
            dot.setRadius(5.f);
            dot.setFillColor(dotCol);
            dot.setPosition({x + 10.f, y + h * .5f - 5.f});
        }
    }

    bool hit(sf::Vector2f p) const
    { return bg.getGlobalBounds().contains(p); }

    void setActive(bool a)
    {
        bg.setOutlineThickness(a ? 1.5f : 0.f);
        bg.setFillColor({(uint8_t)(base.r / (a?2:3)),
                         (uint8_t)(base.g / (a?2:3)),
                         (uint8_t)(base.b / (a?2:3)), 255});
        lbl->setFillColor(a ? sf::Color::White : sf::Color{200,200,218});
    }

    void draw(sf::RenderWindow& w)
    {
        w.draw(bg);
        if (hasDot) w.draw(dot);
        if (lbl)    w.draw(*lbl);
    }
};


int main()
{
    sf::RenderWindow window(
        sf::VideoMode({(unsigned)WIN_W, (unsigned)WIN_H}),
        "Pathfinding Visualizer",
        sf::Style::Close
    );
    window.setFramerateLimit(60);

    sf::View fixedView(sf::FloatRect({0.f,0.f},{WIN_W,WIN_H}));
    window.setView(fixedView);

    sf::Font font;
    if (!font.openFromFile("arial.ttf")) return -1;

    
    const float PX  = GRID_W + 12.f;
    const float PW  = PANEL_W - 24.f;
    const float BH  = 32.f;
    const float BSP = 38.f;
    float cy = 10.f;

    auto makeLabel = [&](const string& s, float x, float y){
        auto t = make_unique<sf::Text>(font, s, 9);
        t->setFillColor(C_LABEL);
        t->setPosition({x, y});
        t->setLetterSpacing(2.2f);
        return t;
    };

   
    auto lblTools = makeLabel("TOOLS", PX, cy); cy += 15.f;

    Button btnWall, btnErase, btnStart, btnEnd;
    btnWall .init(font,"Wall",  PX,cy,PW,BH,{100,140,210},C_WALL);  cy+=BSP;
    btnErase.init(font,"Erase", PX,cy,PW,BH,{120,120,145});          cy+=BSP;
    btnStart.init(font,"Start", PX,cy,PW,BH,{ 46,200,110},C_START); cy+=BSP;
    btnEnd  .init(font,"End",   PX,cy,PW,BH,{220, 70, 55},C_END);   cy+=BSP+2.f;

    float divY1 = cy; cy += 7.f;

    
    auto lblAlgo = makeLabel("ALGORITHMS", PX, cy); cy += 15.f;

    Button btnBFS, btnDFS, btnClear;
    btnBFS  .init(font,"Run BFS",PX,cy,PW,BH,{100,160,255}); cy+=BSP;
    btnDFS  .init(font,"Run DFS",PX,cy,PW,BH,{255,145, 50}); cy+=BSP+2.f;

    float divY2 = cy; cy += 7.f;

    
    auto lblActions = makeLabel("ACTIONS", PX, cy); cy += 15.f;
    btnClear.init(font,"Clear",  PX,cy,PW,BH,{ 80, 80,100}); cy+=BSP+2.f;

    float divY3 = cy; cy += 7.f;

    
    sf::Text statusText(font, "Set start & end", 10);
    statusText.setFillColor(C_STATUS);
    statusText.setPosition({PX, cy}); cy += 18.f;

    
    auto lblStats = makeLabel("STATS", PX, cy); cy += 14.f;

    sf::Text statAlgo (font, "Algo   : --",  10);
    sf::Text statVisit(font, "Visited: 0",   10);
    sf::Text statPath (font, "Path   : --",  10);

    for (auto* t : {&statAlgo, &statVisit, &statPath})
    {
        t->setFillColor({170, 200, 255});
        t->setPosition({PX, cy});
        cy += 14.f;
    }
    cy += 4.f;

   
    auto lblLegend = makeLabel("LEGEND", PX, cy); cy += 14.f;

    struct LE { sf::Color col; string txt; };
    vector<LE> legend = {
        {C_START,      "Start"},
        {C_END,        "End"},
        {C_WALL,       "Wall"},
        {BFS_WAVE[0],  "BFS visited"},
        {DFS_WAVE[0],  "DFS visited"},
        {C_PATH,       "Path"},
    };
    vector<sf::CircleShape>      legDots(legend.size());
    vector<unique_ptr<sf::Text>> legTexts(legend.size());
    for (int i = 0; i < (int)legend.size(); i++)
    {
        legDots[i].setRadius(4.f);
        legDots[i].setFillColor(legend[i].col);
        legDots[i].setPosition({PX, cy + i*13.f + 2.f});
        legTexts[i] = make_unique<sf::Text>(font, legend[i].txt, 10);
        legTexts[i]->setFillColor({170,170,190});
        legTexts[i]->setPosition({PX+13.f, cy + i*13.f});
    }

    
    Button* toolBtns[4] = {&btnWall,&btnErase,&btnStart,&btnEnd};
    auto refreshActive = [&](){
        for (int i=0;i<4;i++) toolBtns[i]->setActive(i==tool);
    };
    refreshActive();

    auto setStatus = [&](const string& s){ statusText.setString(s); };

    const int dx[4] = {-1,1,0,0};
    const int dy[4] = { 0,0,-1,1};

   
    while (window.isOpen())
    {
       
        while (const auto ev = window.pollEvent())
        {
            if (ev->is<sf::Event::Closed>()) window.close();
            if (ev->is<sf::Event::Resized>()) window.setView(fixedView);

            const auto* mp = ev->getIf<sf::Event::MouseButtonPressed>();
            if (!mp || mp->button != sf::Mouse::Button::Left) continue;
            sf::Vector2f mf{(float)mp->position.x,(float)mp->position.y};

            
            if      (btnWall .hit(mf)){ tool=0; refreshActive(); }
            else if (btnErase.hit(mf)){ tool=1; refreshActive(); }
            else if (btnStart.hit(mf)){ tool=2; refreshActive(); }
            else if (btnEnd  .hit(mf)){ tool=3; refreshActive(); }

            else if (btnBFS.hit(mf))
            {
                if (startR==-1 || endR==-1)
                    setStatus("Set start & end first!");
                else if (!algoRunning && !tracing)
                {
                    clearVis();
                    memset(vis,false,sizeof(vis));
                    bfsQ = {}; dfsStk = {};
                    bfsRunning  = true;
                    dfsRunning  = false;
                    algoRunning = true;
                    lastWasDFS  = false;
                    bfsQ.push({startR,startC});
                    vis[startR][startC] = true;
                    algoClock.restart();
                    setStatus("Running BFS...");
                }
            }

            else if (btnDFS.hit(mf))
            {
                if (startR==-1 || endR==-1)
                    setStatus("Set start & end first!");
                else if (!algoRunning && !tracing)
                {
                    clearVis();
                    memset(vis,false,sizeof(vis));
                    bfsQ = {}; dfsStk = {};
                    dfsRunning  = true;
                    bfsRunning  = false;
                    algoRunning = true;
                    lastWasDFS  = true;
                    
                    dfsStk.push({startR,startC});
                    vis[startR][startC] = true;
                    algoClock.restart();
                    setStatus("Running DFS...");
                }
            }

            else if (btnClear.hit(mf))
            {
                stopAll(); clearAll();
                setStatus("Cleared.");
            }

            else
            {
                
                if (algoRunning || tracing) continue;
                int gc = (int)(mf.x/CELL);
                int gr = (int)(mf.y/CELL);
                if (gr<0||gr>=ROWS||gc<0||gc>=COLS) continue;

                if (tool==0 && grid[gr][gc]!=2 && grid[gr][gc]!=3)
                    grid[gr][gc]=1;
                else if (tool==1)
                {
                    if(grid[gr][gc]==2) startR=startC=-1;
                    if(grid[gr][gc]==3) endR  =endC  =-1;
                    grid[gr][gc]=0;
                }
                else if (tool==2)
                {
                    if(startR!=-1) grid[startR][startC]=0;
                    startR=gr; startC=gc; grid[gr][gc]=2;
                    setStatus("Start placed");
                }
                else if (tool==3)
                {
                    if(endR!=-1) grid[endR][endC]=0;
                    endR=gr; endC=gc; grid[gr][gc]=3;
                    setStatus("End placed");
                }
            }
        }

        if (!algoRunning && !tracing &&
            sf::Mouse::isButtonPressed(sf::Mouse::Button::Left))
        {
            auto raw=sf::Mouse::getPosition(window);
            int gc=raw.x/(int)CELL, gr=raw.y/(int)CELL;
            if (gr>=0&&gr<ROWS&&gc>=0&&gc<COLS)
            {
                if (tool==0&&grid[gr][gc]!=2&&grid[gr][gc]!=3)
                    grid[gr][gc]=1;
                else if (tool==1)
                {
                    if(grid[gr][gc]==2) startR=startC=-1;
                    if(grid[gr][gc]==3) endR  =endC  =-1;
                    grid[gr][gc]=0;
                }
            }
        }

        if (bfsRunning && algoClock.getElapsedTime().asSeconds() > ALGO_DT)
        {
            algoClock.restart();
            if (!bfsQ.empty())
            {
                auto [r,c] = bfsQ.front(); bfsQ.pop();
                waveStep++; cellsVisited++;
                if(grid[r][c]!=2&&grid[r][c]!=3)
                { grid[r][c]=4; waveAge[r][c]=waveStep; }

                for (int i=0;i<4;i++)
                {
                    int nr=r+dx[i], nc=c+dy[i];
                    if(nr>=0&&nr<ROWS&&nc>=0&&nc<COLS
                       &&!vis[nr][nc]&&grid[nr][nc]!=1)
                    {
                        vis[nr][nc]=true; par[nr][nc]={r,c};
                        bfsQ.push({nr,nc});
                        if(nr==endR&&nc==endC)
                        {
                            bfsRunning=false; algoRunning=false;
                            tracing=true; pathR=endR; pathC=endC;
                            algoClock.restart();
                            setStatus("BFS: path found!");
                            goto doneSearching;
                        }
                    }
                }
            }
            else { bfsRunning=false; algoRunning=false; setStatus("BFS: no path."); }
        }

        if (dfsRunning && algoClock.getElapsedTime().asSeconds() > ALGO_DT)
        {
            algoClock.restart();
            if (!dfsStk.empty())
            {
                auto [r,c] = dfsStk.top(); dfsStk.pop();  

                waveStep++; cellsVisited++;
                if(grid[r][c]!=2&&grid[r][c]!=3)
                { grid[r][c]=4; waveAge[r][c]=waveStep; }

                for (int i=0;i<4;i++)
                {
                    int nr=r+dx[i], nc=c+dy[i];
                    if(nr>=0&&nr<ROWS&&nc>=0&&nc<COLS
                       &&!vis[nr][nc]&&grid[nr][nc]!=1)
                    {
                        vis[nr][nc]=true; par[nr][nc]={r,c};
                        dfsStk.push({nr,nc});   
                        if(nr==endR&&nc==endC)
                        {
                            dfsRunning=false; algoRunning=false;
                            tracing=true; pathR=endR; pathC=endC;
                            algoClock.restart();
                            setStatus("DFS: path found!");
                            goto doneSearching;
                        }
                    }
                }
            }
            else { dfsRunning=false; algoRunning=false; setStatus("DFS: no path."); }
        }

        doneSearching:;
        if (tracing && algoClock.getElapsedTime().asSeconds() > PATH_DT)
        {
            algoClock.restart();
            if (pathR==startR && pathC==startC)
            {
                tracing=false;
                grid[startR][startC]=2;
                grid[endR  ][endC  ]=3;
                setStatus("Done!");
            }
            else
            {
                if(grid[pathR][pathC]!=3) grid[pathR][pathC]=5;
                auto[pr,pc]=par[pathR][pathC];
                pathR=pr; pathC=pc;
                pathLen++;
            }
        }

        window.clear({10,10,16});

        sf::RectangleShape gridBg({GRID_W,GRID_H});
        gridBg.setFillColor({16,16,22});
        window.draw(gridBg);

        for (int r=0;r<ROWS;r++)
        for (int c=0;c<COLS;c++)
        {
            sf::RectangleShape cell({CELL-GAP,CELL-GAP});
            cell.setPosition({c*CELL+GAP*.5f, r*CELL+GAP*.5f});

            int v=grid[r][c];
            if      (v==0) cell.setFillColor(C_EMPTY);
            else if (v==1) cell.setFillColor(C_WALL);
            else if (v==2) cell.setFillColor(C_START);
            else if (v==3) cell.setFillColor(C_END);
            else if (v==5) cell.setFillColor(C_PATH);
            else if (v==4)
            {
                int age    = waveStep - waveAge[r][c];
                int bucket = min(age/3, 7);
        
                cell.setFillColor(lastWasDFS ? DFS_WAVE[bucket]
                                             : BFS_WAVE[bucket]);
            }
            window.draw(cell);
        }

        sf::RectangleShape gridBorder({GRID_W,GRID_H});
        gridBorder.setFillColor(sf::Color::Transparent);
        gridBorder.setOutlineThickness(1.f);
        gridBorder.setOutlineColor({38,38,52});
        window.draw(gridBorder);

        sf::RectangleShape panel({PANEL_W,WIN_H});
        panel.setPosition({GRID_W,0.f});
        panel.setFillColor(C_PANEL);
        window.draw(panel);

        sf::RectangleShape panelEdge({1.f,WIN_H});
        panelEdge.setPosition({GRID_W,0.f});
        panelEdge.setFillColor({38,38,52});
        window.draw(panelEdge);

        // Dividers
        for (float dy2 : {divY1, divY2, divY3})
        {
            sf::RectangleShape div({PW,1.f});
            div.setPosition({PX,dy2});
            div.setFillColor(C_DIVIDE);
            window.draw(div);
        }

        window.draw(*lblTools);
        window.draw(*lblAlgo);
        window.draw(*lblActions);
        window.draw(*lblLegend);

        btnWall .draw(window);
        btnErase.draw(window);
        btnStart.draw(window);
        btnEnd  .draw(window);
        btnBFS  .draw(window);
        btnDFS  .draw(window);
        btnClear.draw(window);

        window.draw(statusText);

        
        {
            string algoName = !lastWasDFS ? "BFS" : "DFS";
            if (!algoRunning && !tracing && cellsVisited == 0)
                algoName = "--";

            statAlgo .setString("Algo   : " + algoName);
            statVisit.setString("Visited: " + to_string(cellsVisited));

            if (tracing || (!algoRunning && pathLen > 0))
                statPath.setString("Path   : " + to_string(pathLen) + " steps");
            else
                statPath.setString("Path   : --");
        }
        window.draw(*lblStats);
        window.draw(statAlgo);
        window.draw(statVisit);
        window.draw(statPath);

        for (int i=0;i<(int)legend.size();i++)
        {
            window.draw(legDots[i]);
            window.draw(*legTexts[i]);
        }

        window.display();
    }
    return 0;
}