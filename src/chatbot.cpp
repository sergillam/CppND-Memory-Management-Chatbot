#include <algorithm>
#include <ctime>
#include <iostream>
#include <random>

#include "chatbot.h"
#include "chatlogic.h"
#include "graphedge.h"
#include "graphnode.h"

// constructor WITHOUT memory allocation
ChatBot::ChatBot() {
  // invalidate data handles
  _image = nullptr;
  _chatLogic = nullptr;
  _rootNode = nullptr;
}

// constructor WITH memory allocation
ChatBot::ChatBot(std::string filename) {
  std::cout << "ChatBot Constructor" << std::endl;

  // invalidate data handles
  _chatLogic = nullptr;
  _rootNode = nullptr;

  // load image into heap memory
  _image = new wxBitmap(filename, wxBITMAP_TYPE_PNG);
}

ChatBot::~ChatBot() {
  std::cout << "ChatBot Destructor" << std::endl;

  // deallocate heap memory
  if (_image != NULL) {
    delete _image;
    _image = NULL;
  }
}

// copy constructor
ChatBot::ChatBot(const ChatBot &source) {
  std::cout << "Copy Ctor Called for ChatBot\n";
  // Assign data from source
  this->_image = (source._image == nullptr) ? nullptr : new wxBitmap(*source._image);
  this->_currentNode = source._currentNode;
  this->_rootNode = source._rootNode;
  this->_chatLogic = source._chatLogic;
  this->_chatLogic->SetChatbotHandle(this);
}

// copy assignment operator
ChatBot &ChatBot::operator=(const ChatBot &source) {
  std::cout << "ChatBot Copy Operator" << std::endl;

  // Check if source is the same as self
  if (this == &source) {
    return *this;
  }

  // Delete and free memory, if image exists
  // Other pointers are not deleted, because they are not owned by the ChatBot
  // class.
  if (_image != NULL) {
    delete _image;
  }

  // Assign data from source
  if (source._image != NULL) {
    this->_image = new wxBitmap(*source._image);
  } else {
    this->_image = NULL;
  }
  this->_currentNode = source._currentNode;
  this->_rootNode = source._rootNode;
  this->_chatLogic = source._chatLogic;
  this->_chatLogic->SetChatbotHandle(this);

  return *this;
}

// move constructor
ChatBot::ChatBot(ChatBot &&source) {
  std::cout << "ChatBot Move Constructor" << std::endl;

  // Assign data from source
  if (source._image != NULL) {
    this->_image = new wxBitmap(*source._image);
  } else {
    this->_image = NULL;
  }
  this->_currentNode = source._currentNode;
  this->_rootNode = source._rootNode;
  this->_chatLogic = source._chatLogic;
  this->_chatLogic->SetChatbotHandle(this);

  // Remove owned values from source
  source._image = NULL;
}

// move assignment operator
ChatBot &ChatBot::operator=(ChatBot &&source) {
  std::cout << "ChatBot Move Operator" << std::endl;

  // Check if source is the same as self
  if (this == &source) {
    return *this;
  }

  // Remove owned values
  if (this->_image != NULL) {
    delete this->_image;
  }

  // Assign data from source
  if (source._image != NULL) {
    this->_image = new wxBitmap(*source._image);
  } else {
    this->_image = NULL;
  }
  this->_currentNode = source._currentNode;
  this->_rootNode = source._rootNode;
  this->_chatLogic = source._chatLogic;
  this->_chatLogic->SetChatbotHandle(this);

  // Remove owned values from source
  source._image = NULL;

  return *this;
}

void ChatBot::ReceiveMessageFromUser(std::string message) {
  // loop over all edges and keywords and compute Levenshtein distance to query
  typedef std::pair<GraphEdge *, int> EdgeDist;
  std::vector<EdgeDist> levDists; // format is <ptr,levDist>

  for (size_t i = 0; i < _currentNode->GetNumberOfChildEdges(); ++i) {
    GraphEdge *edge = _currentNode->GetChildEdgeAtIndex(i);
    for (auto keyword : edge->GetKeywords()) {
      EdgeDist ed{edge, ComputeLevenshteinDistance(keyword, message)};
      levDists.push_back(ed);
    }
  }

  // select best fitting edge to proceed along
  GraphNode *newNode;
  if (levDists.size() > 0) {
    // sort in ascending order of Levenshtein distance (best fit is at the top)
    std::sort(levDists.begin(), levDists.end(),
              [](const EdgeDist &a, const EdgeDist &b) {
                return a.second < b.second;
              });
    newNode = levDists.at(0).first->GetChildNode(); // after sorting the best
                                                    // edge is at first position
  } else {
    // go back to root node
    newNode = _rootNode;
  }

  // tell current node to move chatbot to new node
  _currentNode->MoveChatbotToNewNode(newNode);
}

void ChatBot::SetCurrentNode(GraphNode *node) {
  // update pointer to current node
  _currentNode = node;

  // select a random node answer (if several answers should exist)
  std::vector<std::string> answers = _currentNode->GetAnswers();
  std::mt19937 generator(int(std::time(0)));
  std::uniform_int_distribution<int> dis(0, answers.size() - 1);
  std::string answer = answers.at(dis(generator));

  // send selected node answer to user
  _chatLogic->SendMessageToUser(answer);
}

int ChatBot::ComputeLevenshteinDistance(std::string s1, std::string s2) {
  // convert both strings to upper-case before comparing
  std::transform(s1.begin(), s1.end(), s1.begin(), ::toupper);
  std::transform(s2.begin(), s2.end(), s2.begin(), ::toupper);

  // compute Levenshtein distance measure between both strings
  const size_t m(s1.size());
  const size_t n(s2.size());

  if (m == 0)
    return n;
  if (n == 0)
    return m;

  size_t *costs = new size_t[n + 1];

  for (size_t k = 0; k <= n; k++)
    costs[k] = k;

  size_t i = 0;
  for (std::string::const_iterator it1 = s1.begin(); it1 != s1.end();
       ++it1, ++i) {
    costs[0] = i + 1;
    size_t corner = i;

    size_t j = 0;
    for (std::string::const_iterator it2 = s2.begin(); it2 != s2.end();
         ++it2, ++j) {
      size_t upper = costs[j + 1];
      if (*it1 == *it2) {
        costs[j + 1] = corner;
      } else {
        size_t t(upper < corner ? upper : corner);
        costs[j + 1] = (costs[j] < t ? costs[j] : t) + 1;
      }

      corner = upper;
    }
  }

  int result = costs[n];
  delete[] costs;

  return result;
}