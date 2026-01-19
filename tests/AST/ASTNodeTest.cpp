#include "AST/ASTNode.h"
#include <cassert>
#include <iostream>

using namespace chtholly;

namespace chtholly {
class MockASTNode : public ASTNode {
public:
    std::string toString() const override { return "MockASTNode"; }
    std::unique_ptr<ASTNode> clone() const override { return std::make_unique<MockASTNode>(); }
    ASTNodeKind getKind() const override { return ASTNodeKind::Block; }
};
}

void testASTNode() {
    std::unique_ptr<ASTNode> node = std::make_unique<MockASTNode>();
    assert(node->toString() == "MockASTNode");
    std::cout << "testASTNode passed!" << std::endl;
}

int main() {
    testASTNode();
    return 0;
}
