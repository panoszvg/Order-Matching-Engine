#pragma once
#include <memory>
#include "Book.h"

int runInsertCancelBenchmark(std::shared_ptr<Book> book);
int runMatchSimulationBenchmark(std::shared_ptr<Book> book);
int runMixedLoadBenchmark(std::shared_ptr<Book> book);