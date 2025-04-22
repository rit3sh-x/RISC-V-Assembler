#include <emscripten/bind.h>
#include <emscripten/val.h>
#include "simulator.hpp"

using namespace emscripten;

val mapToVal(const std::map<uint32_t, std::pair<uint32_t, std::string>>& m) {
    val result = val::object();
    for (const auto& [key, value] : m) {
        val pair = val::object();
        pair.set("first", value.first);
        pair.set("second", value.second);
        result.set(std::to_string(key), pair);
    }
    return result;
}

val activeStageToVal(const std::map<Stage, std::pair<bool, uint32_t>>& m) {
    val result = val::object();
    for (const auto& [stage, data] : m) {
        val stageData = val::object();
        stageData.set("active", data.first);
        stageData.set("instruction", data.second);
        result.set(static_cast<int>(stage), stageData);
    }
    return result;
}

val unorderedMapToVal(const std::unordered_map<uint32_t, uint8_t>& m) {
    val result = val::object();
    for (const auto& [key, value] : m) {
        result.set(std::to_string(key), value);
    }
    return result;
}

val unorderedMapIntStringToVal(const std::unordered_map<int, std::string>& m) {
    val result = val::object();
    for (const auto& [key, value] : m) {
        result.set(std::to_string(key), value);
    }
    return result;
}

val instructionRegistersToVal(const InstructionRegisters& regs) {
    val result = val::object();
    result.set("RA", regs.RA);
    result.set("RB", regs.RB);
    result.set("RM", regs.RM);
    result.set("RY", regs.RY);
    result.set("RZ", regs.RZ);
    return result;
}

val uiResponseToVal(const UIResponse& response) {
    val result = val::object();
    result.set("isFlushed", response.isFlushed);
    result.set("isStalled", response.isStalled);
    result.set("isDataForwarded", response.isDataForwarded);
    result.set("isProgramTerminated", response.isProgramTerminated);
    return result;
}

val simulationStatsToVal(const SimulationStats& stats) {
    val result = val::object();
    result.set("totalCycles", stats.totalCycles);
    result.set("stallBubbles", stats.stallBubbles);
    result.set("pipelineFlushes", stats.pipelineFlushes);
    result.set("dataHazards", stats.dataHazards);
    result.set("controlHazards", stats.controlHazards);
    result.set("dataHazardStalls", stats.dataHazardStalls);
    result.set("controlHazardStalls", stats.controlHazardStalls);
    result.set("aluInstructions", stats.aluInstructions);
    result.set("dataTransferInstructions", stats.dataTransferInstructions);
    result.set("controlInstructions", stats.controlInstructions);
    result.set("cyclesPerInstruction", stats.cyclesPerInstruction);
    result.set("instructionsExecuted", stats.instructionsExecuted);
    result.set("branchPredictionAccuracy", stats.branchPredictionAccuracy);
    return result;
}

val pipelineDiagramInfoToVal(const PipelineDiagramInfo& info) {
    val result = val::object();
    result.set("ExExForwarding", info.ExExForwarding);
    result.set("MemExForwarding", info.MemExForwarding);
    result.set("BranchToFetch", info.BranchToFetch);
    result.set("ExToBranch", info.ExToBranch);
    return result;
}

class SimulatorWrapper {
public:
    SimulatorWrapper() : sim() {}
    
    bool loadProgram(const std::string& input) { 
        return sim.loadProgram(input);
    }
    
    bool step() { 
        return sim.step();
    }
    
    void run() { 
        sim.run();
    }

    void reset() {
        sim.reset();
    }

    val getRegisters() {
        const uint32_t* regs = sim.getRegisters();
        val result = val::array();
        for (int i = 0; i < NUM_REGISTERS; i++) {
            result.set(i, regs[i]);
        }
        return result;
    }
    
    uint32_t getPC() const { return sim.getPC(); }
    uint32_t getCycles() const { return sim.getCycles(); }
    val getDataMap() const { return unorderedMapToVal(sim.getDataMap()); }
    val getTextMap() const { return mapToVal(sim.getTextMap()); }
    val getLogs() { return unorderedMapIntStringToVal(sim.getLogs()); }
    bool isRunning() const { return sim.isRunning(); }
    
    val getActiveStages() const { 
        return activeStageToVal(sim.getActiveStages());
    }
    
    uint32_t getStalls() const { return sim.getStalls(); }
    void setEnvironment(bool pipeline, bool dataForwarding) {
        sim.setEnvironment(pipeline, dataForwarding);
    }

    val getUIResponse() const {
        return uiResponseToVal(sim.getUIResponse());
    }

    val getInstructionRegisters() const {
        return instructionRegistersToVal(sim.getInstructionRegisters());
    }

    val getStats() {
        return simulationStatsToVal(sim.Stats());
    }

    val getPipelineDiagramInfo() const {
        return pipelineDiagramInfoToVal(sim.getPipelineDiagramInfo());
    }

private:
    Simulator sim;
};

EMSCRIPTEN_BINDINGS(simulator_module) {
    enum_<Stage>("Stage")
        .value("FETCH", Stage::FETCH)
        .value("DECODE", Stage::DECODE)
        .value("EXECUTE", Stage::EXECUTE)
        .value("MEMORY", Stage::MEMORY)
        .value("WRITEBACK", Stage::WRITEBACK);

    value_object<InstructionRegisters>("InstructionRegisters")
        .field("RA", &InstructionRegisters::RA)
        .field("RB", &InstructionRegisters::RB)
        .field("RM", &InstructionRegisters::RM)
        .field("RY", &InstructionRegisters::RY)
        .field("RZ", &InstructionRegisters::RZ);

    value_object<UIResponse>("UIResponse")
        .field("isFlushed", &UIResponse::isFlushed)
        .field("isStalled", &UIResponse::isStalled)
        .field("isDataForwarded", &UIResponse::isDataForwarded)
        .field("isProgramTerminated", &UIResponse::isProgramTerminated);
        
    value_object<SimulationStats>("SimulationStats")
        .field("totalCycles", &SimulationStats::totalCycles)
        .field("stallBubbles", &SimulationStats::stallBubbles)
        .field("pipelineFlushes", &SimulationStats::pipelineFlushes)
        .field("dataHazards", &SimulationStats::dataHazards)
        .field("controlHazards", &SimulationStats::controlHazards)
        .field("dataHazardStalls", &SimulationStats::dataHazardStalls)
        .field("controlHazardStalls", &SimulationStats::controlHazardStalls)
        .field("aluInstructions", &SimulationStats::aluInstructions)
        .field("dataTransferInstructions", &SimulationStats::dataTransferInstructions)
        .field("controlInstructions", &SimulationStats::controlInstructions)
        .field("cyclesPerInstruction", &SimulationStats::cyclesPerInstruction)
        .field("instructionsExecuted", &SimulationStats::instructionsExecuted)
        .field("branchPredictionAccuracy", &SimulationStats::branchPredictionAccuracy);

    value_object<PipelineDiagramInfo>("PipelineDiagramInfo")
        .field("ExExForwarding", &PipelineDiagramInfo::ExExForwarding)
        .field("MemExForwarding", &PipelineDiagramInfo::MemExForwarding)
        .field("BranchToFetch", &PipelineDiagramInfo::BranchToFetch)
        .field("ExToBranch", &PipelineDiagramInfo::ExToBranch);
        
    class_<SimulatorWrapper>("Simulator")
        .constructor<>()
        .function("loadProgram", &SimulatorWrapper::loadProgram)
        .function("step", &SimulatorWrapper::step)
        .function("run", &SimulatorWrapper::run)
        .function("reset", &SimulatorWrapper::reset)
        .function("getRegisters", &SimulatorWrapper::getRegisters)
        .function("getPC", &SimulatorWrapper::getPC)
        .function("getCycles", &SimulatorWrapper::getCycles)
        .function("getDataMap", &SimulatorWrapper::getDataMap)
        .function("getTextMap", &SimulatorWrapper::getTextMap)
        .function("getLogs", &SimulatorWrapper::getLogs)
        .function("isRunning", &SimulatorWrapper::isRunning)
        .function("getActiveStages", &SimulatorWrapper::getActiveStages)
        .function("getStalls", &SimulatorWrapper::getStalls)
        .function("setEnvironment", &SimulatorWrapper::setEnvironment)
        .function("getInstructionRegisters", &SimulatorWrapper::getInstructionRegisters)
        .function("getUIResponse", &SimulatorWrapper::getUIResponse)
        .function("getStats", &SimulatorWrapper::getStats)
        .function("getPipelineDiagramInfo", &SimulatorWrapper::getPipelineDiagramInfo);
}