//
// Created by Sheldon Woodward on 4/27/18.
//

#include "ANN.hpp"

// constructor
ANN::ANN(int inputNum, int outputNum, std::string species) {
    this->nodes = std::deque<Node>();
    this->genome = std::deque<ConnectionGene>();

    this->inputNodes = std::deque<Node*>();

    this->outputNodes = std::deque<Node*>();
    this->layerSortedNodes = std::deque<Node*>();
    this->nonInputLayerSortedNodes = std::deque<Node*>();
    this->sequentialSortedNodes = std::deque<Node*>();
    this->enabledSortedGenome = std::deque<ConnectionGene*>();

    this->weightMatrix = std::deque<std::deque<float>>();
    this->inputVector = std::deque<float*>();

    this->species = std::move(species);
    this->layerCount = 1;

    // add inputs
    for (int i = 0; i < inputNum; i++) {
        nodes.emplace_back((int)nodes.size());
        inputNodes.push_back(&nodes.back());
    }

    // add outputs
    for (int o = 0; o < outputNum; o++) {
        nodes.emplace_back((int)nodes.size());
        outputNodes.push_back(&nodes.back());
    }

    // add input to output connections
    for (auto in : inputNodes) {
        for (auto on : outputNodes) {
            genome.emplace_back(in, on, randomWeight(), true, genome.size());
        }
    }

    setup();
}

// set get
std::deque<ConnectionGene> ANN::getGenome() {
    return genome;
}

std::string ANN::getSpecies() {
    return species;
}

// setup functions
void ANN::setup() {
    sortNodes();
    determineLayers();
    sortGenome();
    determineWeightMatrix();
}

void ANN::sortNodes() {
    layerSortedNodes = std::deque<Node*>();
    nonInputLayerSortedNodes = std::deque<Node*>();
    sequentialSortedNodes = std::deque<Node*>();
    for (auto &node : nodes) {
        layerSortedNodes.push_back(&node);
        sequentialSortedNodes.push_back(&node);
        if (std::find(inputNodes.begin(), inputNodes.end(), &node) == inputNodes.end()) {
            nonInputLayerSortedNodes.push_back(&node);
        }
    }
    std::sort(layerSortedNodes.begin(), layerSortedNodes.end(), Node::ptrLayerSort);
    std::sort(nonInputLayerSortedNodes.begin(), nonInputLayerSortedNodes.end(), Node::ptrLayerSort);
    std::sort(sequentialSortedNodes.begin(), sequentialSortedNodes.end(), Node::nodeNumSort);
}

void ANN::sortGenome() {
    enabledSortedGenome = std::deque<ConnectionGene*>();
    for (unsigned long cg = 0; cg < genome.size(); cg++) {
        if (genome[cg].getEnabled()) {
            enabledSortedGenome.push_back(&genome.at(cg));
        }
    }
    std::sort(enabledSortedGenome.begin(), enabledSortedGenome.end(), ConnectionGene::ptrComparison);
}

void ANN::determineLayers() {
    for (auto &on : outputNodes) {
        determineLayers(on, 0);
    }
}

void ANN::determineLayers(Node* node, unsigned int layer) {
    if (node->getLayer() <= layer && std::find(outputNodes.begin(), outputNodes.end(), node) == outputNodes.end()) {
        node->setLayer(layer + 1);
        if (layerCount <= layer + 1) {
            layerCount = layer + 1;
        }
    }
    for (auto &cg : genome) {
        if (cg.getTo() == node) {
            cg.setLayer(node->getLayer());
            determineLayers(cg.getFrom(), node->getLayer());
        }
    }
    if (std::find(inputNodes.begin(), inputNodes.end(), node) != inputNodes.end()) {
        node->setLayer(layerCount);
    }
}

void ANN::determineWeightMatrix() {
    weightMatrix = std::deque<std::deque<float>>();
    for (int row = 0; row < nodes.size(); row++) {
        weightMatrix.emplace_back(nodes.size(), 0.0f);
    }

    // build weight matrix
//    auto sortGenome = getEnabledSortedGenome();
    for (auto &cg : enabledSortedGenome) {
        weightMatrix[cg->getTo()->getNodeNum()][cg->getFrom()->getNodeNum()] = cg->getWeight();
    }
}

// mutations
void ANN::addNodeMutation() {
//    std::deque<ConnectionGene*> enabledConnections = getEnabledSortedGenome();
    ConnectionGene* randomConnection = enabledSortedGenome.at(rand() % enabledSortedGenome.size());
    // TODO: could create duplicate layers
    nodes.emplace_back((int)nodes.size());
    // TODO: check if innovation exists
    genome.emplace_back(randomConnection->getFrom(), &nodes.back(), randomWeight(), true, genome.size());
    genome.emplace_back(&nodes.back(), randomConnection->getTo(), randomWeight(), true, genome.size());
    randomConnection->setEnabled(false);
    setup();
}

void ANN::addConnectionMutation() {
    // TODO: check that good connections are produced
    std::deque<ConnectionGene> possibleConnections = std::deque<ConnectionGene>();
    for (unsigned long n1 = 0; n1 < nodes.size(); n1++) {
        for (unsigned long n0 = 0; n0 < nodes.size(); n0++) {
            if (nodes[n0].getLayer() > nodes[n1].getLayer() && findConnection(&nodes.at(n0), &nodes.at(n1)) == nullptr) {
                // TODO: check if innovation exists
                possibleConnections.emplace_back(&nodes.at(n0), &nodes.at(n1), randomWeight(), true, genome.size());
            }
        }
    }
    if (possibleConnections.empty()) {
        return;
    }
    genome.push_back(possibleConnections[rand() % possibleConnections.size()]);
    setup();
}

// computation
std::deque<float> ANN::compute(std::deque<float> inputs) {
    // set inputs
    if (inputs.size() != inputNodes.size()) return std::deque<float>(outputNodes.size(), 0.0f);
    for (int i = 0; i < inputs.size(); i++) inputNodes[i]->setValue(inputs[i]);

    // set inputVector
    inputVector = std::deque<float*>();
    for (auto &node : sequentialSortedNodes) inputVector.push_back(node->getValuePtr());

    // feed network
    for (auto &node : nonInputLayerSortedNodes) {
        float* currentInput = inputVector[node->getNodeNum()];
        *currentInput = 0.0;
        for (int n = 0; n < nodes.size(); n++) {
            *currentInput += weightMatrix[node->getNodeNum()][n] * *inputVector[n];
        }

        // activation function
        if (*currentInput < 0.0f) *currentInput = *currentInput / 100.0f;
    }

    // gather outputs
    std::deque<float> outputs = std::deque<float>();
    for (auto &node : outputNodes) outputs.push_back(std::max(node->getValue(), 0.0f)); // output activation
    return outputs;
}

// general
float ANN::randomWeight() {
    return (float)(rand() % 1000) / 1000.0f;
//    return 1.0f;
}

ConnectionGene* ANN::findConnection(Node *from, Node *to) {
    for (unsigned long cg = 0; cg < genome.size(); cg++) {
        if (genome[cg].getFrom() == from && genome[cg].getTo() == to) return &genome.at(cg);
    }
    return nullptr;
}
