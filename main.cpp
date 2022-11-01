#include <unordered_map>
#include <string>
#include <fstream>
#include <iostream>
#include <list>
#include <thread>
#include <ctime>
#include <mutex>
#include <random>

#define CREATOR_THREAD_COUNT 5
#define CONSISTENCY_THREAD_COUNT 6

#define NUMBER_OF_BANK_ACCOUNTS 6

typedef struct {
    int id;
    int senderId;
    int receiverId;
    int amount;
} OPERATION;

typedef struct {
    int id;
    int balance;
    std::vector<OPERATION> operations;
} BANK_ACCOUNT;

std::unordered_map<int, BANK_ACCOUNT> _bankAccounts;
std::vector<OPERATION> _operations;
int _nextOperationId = 0;

std::mutex *bankAccountsMutex;


std::unordered_map<int, BANK_ACCOUNT> readAllBankAccounts(const std::string &filePath) {
    std::unordered_map<int, BANK_ACCOUNT> bankAccounts;
    std::ifstream file(filePath);
    // std::cout << file.is_open();
    BANK_ACCOUNT bankAccount;
    while (file >> bankAccount.id >> bankAccount.balance) {
        bankAccounts.insert({bankAccount.id, bankAccount});
    }
    return bankAccounts;
}

void printOperation(OPERATION &o){
    std::cout<<"OPERATION {id: "<<o.id<<"; senderId: "<<o.senderId<<"; receiverId: "<<o.receiverId<<"; amount: "<< o.amount<<"}\n";
}

void printBankAccount(BANK_ACCOUNT &b){
    std::cout<<"BANK ACCOUNT id: "<<b.id<<"; balance: "<<b.balance<<"; operations: ";
    for(int i=0; i<b.operations.size(); i++){
        printOperation(b.operations[i]);
    }
    std::cout<<'\n';
}

void printAllBankAccounts(){
    for(int i=0; i<_bankAccounts.size(); i++){
        printBankAccount(_bankAccounts[i]);
    }
}

int generateRandomNumberInRange(int min, int max) {
    std::random_device rd;
    std::mt19937 mt(rd());
    std::uniform_int_distribution<int> distribution(min, max);
    return distribution(mt);
}

void transferMoney(BANK_ACCOUNT &a1, BANK_ACCOUNT &a2, int amount) {
    if(a1.balance < amount){
        std::cout<<"Bank account "<<a1.id<<" has insufficient funds.";
        return;
    }
    OPERATION op;
    op.id = _nextOperationId;
    _nextOperationId++;
    op.senderId = a1.id;
    op.receiverId = a2.id;
    op.amount = amount;

    a1.balance -= amount;
    a1.operations.push_back(op);
    a2.balance += amount;
    a2.operations.push_back(op);

    _operations.push_back(op);
}

void createOperation(){
    int firstAccountId = generateRandomNumberInRange(0, 6);
    bankAccountsMutex[firstAccountId].lock();
    int secondAccountId = generateRandomNumberInRange(0, 6);
    while(secondAccountId == firstAccountId) {
        secondAccountId = generateRandomNumberInRange(0, 6);
    }
    bankAccountsMutex[secondAccountId].lock();

    int amount = generateRandomNumberInRange(0, 100);
    std::cout<<"\n======== STARTING OPERATION ========\n";
    std::cout<<"First id generated: "<<firstAccountId<<"\n";
    std::cout<<"Second id generated: "<<secondAccountId<<"\n";
    std::cout<<"Amount generated: "<<amount<<"\n";

    printBankAccount(_bankAccounts[firstAccountId]);
    printBankAccount(_bankAccounts[secondAccountId]);

    transferMoney(_bankAccounts[firstAccountId], _bankAccounts[secondAccountId], amount);

    printBankAccount(_bankAccounts[firstAccountId]);
    printBankAccount(_bankAccounts[secondAccountId]);

    for(int i=0; i<_operations.size(); i++){
        printOperation(_operations[i]);
    }

    std::cout<<"===== OPERATION ENDED ========\n";

    bankAccountsMutex[firstAccountId].unlock();
    bankAccountsMutex[secondAccountId].unlock();

}

void checkAccounts(){

    int consistencyFlag = true;
    int otherAccountFlag = true;
    for(int i=0; i<_bankAccounts.size(); i++){
        int sum = 1000;
        bankAccountsMutex[i].lock();
        BANK_ACCOUNT currentAcc = _bankAccounts[i];
        for(int j=0; j<currentAcc.operations.size(); j++){
            OPERATION currentOp = currentAcc.operations[j];
            int otherAccId;
            if(currentOp.senderId == currentAcc.id){
                sum -= currentOp.amount;
                otherAccId = currentOp.receiverId;
            }
            else if(currentOp.receiverId == currentAcc.id){
                sum += currentOp.amount;
                otherAccId = currentOp.senderId;
            }
            BANK_ACCOUNT otherAcc = _bankAccounts[otherAccId];
            int foundOperation = false;
            for(int k=0; k<otherAcc.operations.size(); k++){
                if (otherAcc.operations[k].id == currentOp.id)
                    foundOperation = true;
            }
            if (!foundOperation)
                otherAccountFlag = false;
        }
        if(sum != currentAcc.balance) {
            consistencyFlag = false;
        //    std::cout << "Account id " << currentAcc.id << " is INCONSISTENT!\n";
        }
        else{
            consistencyFlag = true;
        //    std::cout<<"Account id "<<currentAcc.id<<" is consistent.\n";
        }
        bankAccountsMutex[i].unlock();
    }
    if (consistencyFlag)
        std::cout<<"<<<<<======== CONSISTENCY CHECK PASSED (1)\n";
    else
        std::cout<<">>>>>======== CONSISTENCY CHECK NOT PASSED (1)\n";
    if (otherAccountFlag)
        std::cout<<"<<<<<======== CONSISTENCY CHECK PASSED (2)\n";
    else
        std::cout<<">>>>>======== CONSISTENCY CHECK NOT PASSED (2)\n";

}

int main(){
    std::srand(std::time(nullptr));
    _bankAccounts = readAllBankAccounts("C:\\Users\\Ionut\\CLionProjects\\parallel-and-distributed-programming-\\bankaccounts.txt");
    printAllBankAccounts();

    std::thread creatorThreads[CREATOR_THREAD_COUNT];
    std::thread consistencyThreads[CONSISTENCY_THREAD_COUNT];
    bankAccountsMutex = new std::mutex[_bankAccounts.size()];

    // createOperation();
    // checkAccounts();

    for (int index = 0; index < CREATOR_THREAD_COUNT; index++) {
        creatorThreads[index] = std::thread(createOperation);
    }

    for (int index = 0; index < CONSISTENCY_THREAD_COUNT; index++) {
        consistencyThreads[index] = std::thread(checkAccounts);
    }

    for (int index = 0; index < CREATOR_THREAD_COUNT; index++) {
        creatorThreads[index].join();
    }

    for (int index = 0; index < CONSISTENCY_THREAD_COUNT; index++) {
        consistencyThreads[index].join();
    }

    delete[] bankAccountsMutex;

    return 0;


    /*
    int number_of_bank_accounts = 10;
    BANK_ACCOUNT a1, a2;
    a1.id =  1;
    a2.id = 2;
    a1.balance = 500;
    a2.balance = 1000;

    printBankAccount(a1);
    printBankAccount(a2);

    transferMoney(a1, a2, 200);

    printBankAccount(a1);
    printBankAccount(a2);

    transferMoney(a2, a1, 300);

    printBankAccount(a1);
    printBankAccount(a2);

    for(int i=0; i<_operations.size(); i++){
        printOperation(_operations[i]);
    }

    return 0;
     */
}



/*
std::mutex *productsMutex;
std::mutex test;
std::mutex billMutex;

void printAllProducts(const std::unordered_map<int, PRODUCT> &products) {
    for (auto const &product : products) {
        auto currentProduct = product.second;
        std::cout <<
                  "id: " << currentProduct.id
                  << "  price: " << currentProduct.price
                  << " quantity: " << currentProduct.quantity << std::endl;
    }
}

void createSale() {

    int numberOfProducts = generateRandomNumberInRange(0, _products.size());
    BILL bill;
    bill.price = 0;
    OPERATION operation;
    int operationPrice;
    for (int index = 0; index < numberOfProducts; index++) {
        int productId = generateRandomNumberInRange(1, _products.size());
        productsMutex[productId - 1].lock();
        int quantity = generateRandomNumberInRange(0, _products[productId].quantity - 1);
        if (_products[productId].quantity > quantity && quantity != 0) {
            _products[productId].quantity -= quantity;
            operation.productId = productId;
            operation.quantity = quantity;
            bill.operations.push_back(operation);
            operationPrice = quantity * _products[productId].price;
            _amountOfMoney += operationPrice;
            bill.price += operationPrice;
        }
        productsMutex[productId - 1].unlock();
    }

    if (!bill.operations.empty()) {
        bill.id = _nextBillId++;
        billMutex.lock();
        _bills.push_back(bill);
        billMutex.unlock();
    }
}

void printAllBills() {
    billMutex.lock();
    for (auto const & bill: _bills) {
        std::cout << "====================================" << std::endl;
        std::cout << "<<<<<<<<<<BILL>>>>>>>>>" << std::endl;
        std::cout <<  "id: " << bill.id << " " << std::endl;
        std::cout << "------------------------------------" << std::endl;
        for (auto const & operation: bill.operations) {
            std::cout
                    << "product id: " << operation.productId
                    << " quantity: " << operation.quantity << std::endl;
        }
        std::cout << "------------------------------------" << std::endl;
        std::cout << "Total amount:         " << bill.price << std::endl;
        std::cout << "====================================" << std::endl << std::endl;
    }
    billMutex.unlock();
}
*/