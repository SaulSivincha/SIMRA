#include "simra/core.hpp"
#include <cassert>
#include <barrier>
#include <iostream>
#include <thread>
using namespace simra;
int main(){
    Position a{1,2};assert((a+Position{2,3}==Position{3,5}));Matrix<int> grid(8,8,0);assert(grid.at({0,0})==0);bool threw=false;try{grid.at({8,0});}catch(const std::out_of_range&){threw=true;}assert(threw);
    Matrix<uint8_t> blocked(8,8,0);assert(bfs(blocked,{0,0},{0,0})->empty());blocked.at({1,0})=1;assert(bfs(blocked,{0,0},{2,0})->size()==4);for(int y=0;y<8;++y)blocked.at({1,y})=1;assert(!bfs(blocked,{0,0},{2,0}));
    LightEquipmentFactory light;IndustrialEquipmentFactory heavy;assert(light.createRobot()->capacity()==10);assert(heavy.createRobot()->capacity()==50);assert(light.charger()!=heavy.charger());
    DeliveryOrderCreator delivery;RestockOrderCreator restock;assert(delivery.create("O1","K1","D1",1)->kind()==OrderKind::Delivery);assert(restock.create("O2","K2","S1",1)->kind()==OrderKind::Restock);
    BasicScenarioBuilder builder;ScenarioDirector director;auto scenario=director.construct(builder,"test");auto clone=scenario.clone();clone->robots[0].battery=20;assert(scenario.robots[0].battery==100);
    WarehouseState state(scenario);assert(state.addOrder(delivery.create("O1","K01","D01",1)));std::barrier sync(3);std::optional<std::string> x,y;
    std::thread t1([&]{sync.arrive_and_wait();x=state.assign("R01");});std::thread t2([&]{sync.arrive_and_wait();y=state.assign("R02");});sync.arrive_and_wait();t1.join();t2.join();assert(bool(x)!=bool(y));
    auto snapshot=state.snapshot();assert(snapshot.orders[0].status==OrderStatus::Assigned);
    ThreadSafeQueue<int> queue;assert(queue.push(4));queue.close();assert(!queue.push(5));assert(queue.waitPop()==4);assert(!queue.waitPop());
    assert(&SimulationLogger::instance()==&SimulationLogger::instance());
    for(auto name:{"basic","mixed","congestion","charging"}){
        auto loaded=loadScenario(std::string(SIMRA_SOURCE_DIR)+"/scenarios/"+name+".json");
        loaded.validate();
        auto path=std::string(SIMRA_SOURCE_DIR)+"/build/roundtrip.json";
        saveScenario(loaded,path);auto roundtrip=loadScenario(path);assert(roundtrip.robots.size()==loaded.robots.size());
    }
    SimulationEngine engine(scenario);engine.start();engine.pause(true);assert(engine.state().snapshot().paused);engine.stop();assert(engine.state().snapshot().stopped);
    std::cout<<"SIMRA core tests passed\n";
}
