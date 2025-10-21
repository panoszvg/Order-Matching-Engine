#pragma once
#include <memory>
#include "Book.h"

int runInsertCancelBenchmark(Book& book);
int runMatchSimulationBenchmark(Book& book);
int runMixedLoadBenchmark(Book& book);