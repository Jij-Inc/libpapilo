#include "libpapilo.h"

#include "papilo/core/Problem.hpp"
#include "papilo/core/ProblemBuilder.hpp"
#include "papilo/core/Presolve.hpp"
#include "papilo/misc/Vec.hpp"

#include <memory>
#include <cstring>
#include <limits>
#include <iostream>

using namespace papilo;

struct papilo_t {
    std::unique_ptr<ProblemBuilder<double>> builder;
    std::unique_ptr<Problem<double>> problem;
    PresolveOptions options;
    int nrows;
    int ncols;
    int nnz;
    bool problem_built;
    
    papilo_t() : nrows(0), ncols(0), nnz(0), problem_built(false) {
        options = PresolveOptions();
    }
};

struct papilo_result_t {
    std::unique_ptr<Problem<double>> presolved_problem;
    papilo_status_t status;
    int deleted_cols;
    int deleted_rows;
    int fixed_cols;
    double presolve_time;
    
    papilo_result_t() : status(PAPILO_STATUS_ERROR), 
                        deleted_cols(0), deleted_rows(0), 
                        fixed_cols(0), presolve_time(0.0) {}
};


extern "C" {

papilo_t* papilo_create(void) {
    try {
        return new papilo_t();
    } catch (...) {
        return nullptr;
    }
}

void papilo_free(papilo_t* papilo) {
    delete papilo;
}

int papilo_set_problem_dimensions(papilo_t* papilo, int nrows, int ncols, int nnz) {
    if (!papilo || nrows < 0 || ncols < 0 || nnz < 0) {
        return PAPILO_ERROR_INVALID_PARAMETER;
    }
    
    if (papilo->problem_built) {
        return PAPILO_ERROR_INVALID_STATE;
    }
    
    try {
        papilo->nrows = nrows;
        papilo->ncols = ncols;
        papilo->nnz = nnz;
        papilo->builder = std::make_unique<ProblemBuilder<double>>();
        papilo->builder->reserve(nnz, nrows, ncols);
        
        // Set the number of rows and columns
        papilo->builder->setNumRows(nrows);
        papilo->builder->setNumCols(ncols);
        
        // Initialize objective coefficients to zero
        for (int i = 0; i < ncols; ++i) {
            papilo->builder->setObj(i, 0.0);
        }
        
        // Initialize bounds to default values
        for (int i = 0; i < ncols; ++i) {
            papilo->builder->setColLb(i, 0.0);
            papilo->builder->setColUb(i, std::numeric_limits<double>::infinity());
        }
        
        for (int i = 0; i < nrows; ++i) {
            papilo->builder->setRowLhs(i, -std::numeric_limits<double>::infinity());
            papilo->builder->setRowRhs(i, std::numeric_limits<double>::infinity());
        }
        
        return PAPILO_OK;
    } catch (...) {
        return PAPILO_ERROR_OUT_OF_MEMORY;
    }
}

int papilo_set_objective(papilo_t* papilo, const double* coefficients, double offset) {
    if (!papilo || !coefficients) {
        return PAPILO_ERROR_INVALID_PARAMETER;
    }
    
    if (papilo->problem_built) {
        return PAPILO_ERROR_INVALID_STATE;
    }
    
    if (!papilo->builder) {
        return PAPILO_ERROR_INVALID_PARAMETER;
    }
    
    try {
        // Set individual coefficients instead of using setObjAll
        for (int i = 0; i < papilo->ncols; ++i) {
            papilo->builder->setObj(i, coefficients[i]);
        }
        papilo->builder->setObjOffset(offset);
        return PAPILO_OK;
    } catch (...) {
        return PAPILO_ERROR_OUT_OF_MEMORY;
    }
}

int papilo_set_col_bounds(papilo_t* papilo, int col, double lb, double ub) {
    if (!papilo || !papilo->builder || col < 0 || col >= papilo->ncols) {
        return PAPILO_ERROR_INVALID_PARAMETER;
    }
    
    if (papilo->problem_built) {
        return PAPILO_ERROR_INVALID_STATE;
    }
    
    try {
        papilo->builder->setColLb(col, lb);
        papilo->builder->setColUb(col, ub);
        return PAPILO_OK;
    } catch (...) {
        return PAPILO_ERROR_OUT_OF_MEMORY;
    }
}

int papilo_set_col_bounds_all(papilo_t* papilo, const double* lb, const double* ub) {
    if (!papilo || !papilo->builder || !lb || !ub) {
        return PAPILO_ERROR_INVALID_PARAMETER;
    }
    
    if (papilo->problem_built) {
        return PAPILO_ERROR_INVALID_STATE;
    }
    
    try {
        for (int i = 0; i < papilo->ncols; ++i) {
            papilo->builder->setColLb(i, lb[i]);
            papilo->builder->setColUb(i, ub[i]);
        }
        return PAPILO_OK;
    } catch (...) {
        return PAPILO_ERROR_OUT_OF_MEMORY;
    }
}

int papilo_set_row_bounds(papilo_t* papilo, int row, double lhs, double rhs) {
    if (!papilo || !papilo->builder || row < 0 || row >= papilo->nrows) {
        return PAPILO_ERROR_INVALID_PARAMETER;
    }
    
    if (papilo->problem_built) {
        return PAPILO_ERROR_INVALID_STATE;
    }
    
    try {
        papilo->builder->setRowLhs(row, lhs);
        papilo->builder->setRowRhs(row, rhs);
        return PAPILO_OK;
    } catch (...) {
        return PAPILO_ERROR_OUT_OF_MEMORY;
    }
}

int papilo_set_row_bounds_all(papilo_t* papilo, const double* lhs, const double* rhs) {
    if (!papilo || !papilo->builder || !lhs || !rhs) {
        return PAPILO_ERROR_INVALID_PARAMETER;
    }
    
    if (papilo->problem_built) {
        return PAPILO_ERROR_INVALID_STATE;
    }
    
    try {
        for (int i = 0; i < papilo->nrows; ++i) {
            papilo->builder->setRowLhs(i, lhs[i]);
            papilo->builder->setRowRhs(i, rhs[i]);
        }
        return PAPILO_OK;
    } catch (...) {
        return PAPILO_ERROR_OUT_OF_MEMORY;
    }
}

int papilo_add_entry(papilo_t* papilo, int row, int col, double value) {
    if (!papilo || 
        row < 0 || row >= papilo->nrows || 
        col < 0 || col >= papilo->ncols) {
        return PAPILO_ERROR_INVALID_PARAMETER;
    }
    
    if (papilo->problem_built) {
        return PAPILO_ERROR_INVALID_STATE;
    }
    
    if (!papilo->builder) {
        return PAPILO_ERROR_INVALID_PARAMETER;
    }
    
    try {
        papilo->builder->addEntry(row, col, value);
        return PAPILO_OK;
    } catch (...) {
        return PAPILO_ERROR_OUT_OF_MEMORY;
    }
}

int papilo_add_entries(papilo_t* papilo, int count, const int* rows, const int* cols, const double* values) {
    if (!papilo || !papilo->builder || count < 0 || !rows || !cols || !values) {
        return PAPILO_ERROR_INVALID_PARAMETER;
    }
    
    if (papilo->problem_built) {
        return PAPILO_ERROR_INVALID_STATE;
    }
    
    try {
        Vec<Triplet<double>> triplets;
        triplets.reserve(count);
        for (int i = 0; i < count; ++i) {
            if (rows[i] < 0 || rows[i] >= papilo->nrows || 
                cols[i] < 0 || cols[i] >= papilo->ncols) {
                return PAPILO_ERROR_INVALID_PARAMETER;
            }
            triplets.emplace_back(rows[i], cols[i], values[i]);
        }
        papilo->builder->addEntryAll(triplets);
        return PAPILO_OK;
    } catch (...) {
        return PAPILO_ERROR_OUT_OF_MEMORY;
    }
}

int papilo_build_problem(papilo_t* papilo) {
    if (!papilo || !papilo->builder) {
        return PAPILO_ERROR_INVALID_PARAMETER;
    }
    
    if (papilo->problem_built) {
        return PAPILO_ERROR_INVALID_STATE;
    }
    
    try {
        papilo->problem = std::make_unique<Problem<double>>(papilo->builder->build());
        papilo->problem_built = true;
        
        // Debug: verify problem dimensions after build (comment out for clean output)
        // std::cerr << "After build - rows: " << papilo->problem->getNRows() 
        //           << ", cols: " << papilo->problem->getNCols() 
        //           << ", nnz: " << papilo->problem->getConstraintMatrix().getNnz() << std::endl;
        
        papilo->builder.reset();  // Free builder memory
        return PAPILO_OK;
    } catch (...) {
        return PAPILO_ERROR_OUT_OF_MEMORY;
    }
}

papilo_result_t* papilo_presolve(papilo_t* papilo) {
    if (!papilo || !papilo->problem || !papilo->problem_built) {
        return nullptr;
    }
    
    try {
        auto result = std::make_unique<papilo_result_t>();
        
        // Create presolve instance
        Presolve<double> presolve;
        presolve.addDefaultPresolvers();
        presolve.getPresolveOptions() = papilo->options;
        
        // Setup problem copy for presolving
        result->presolved_problem = std::make_unique<Problem<double>>(*papilo->problem);
        
        // Debug: print problem dimensions (comment out for clean output)
        // std::cerr << "Original problem - rows: " << papilo->problem->getNRows() 
        //           << ", cols: " << papilo->problem->getNCols() 
        //           << ", nnz: " << papilo->problem->getConstraintMatrix().getNnz() << std::endl;
        
        // Create statistics and numerical objects
        Statistics stats{};
        Num<double> num{};
        Message msg{};
        msg.setVerbosityLevel(VerbosityLevel::kQuiet);
        
        // Run presolve
        PresolveResult<double> presolve_result = presolve.apply(*result->presolved_problem);
        
        // Extract results
        switch (presolve_result.status) {
            case PresolveStatus::kUnchanged:
                result->status = PAPILO_STATUS_UNCHANGED;
                break;
            case PresolveStatus::kReduced:
                result->status = PAPILO_STATUS_REDUCED;
                break;
            case PresolveStatus::kInfeasible:
                result->status = PAPILO_STATUS_INFEASIBLE;
                break;
            case PresolveStatus::kUnbounded:
                result->status = PAPILO_STATUS_UNBOUNDED;
                break;
            case PresolveStatus::kUnbndOrInfeas:
                result->status = PAPILO_STATUS_UNBOUNDED_OR_INFEASIBLE;
                break;
            default:
                result->status = PAPILO_STATUS_ERROR;
                break;
        }
        
        // Store statistics
        result->deleted_cols = papilo->ncols - result->presolved_problem->getNCols();
        result->deleted_rows = papilo->nrows - result->presolved_problem->getNRows();
        result->fixed_cols = 0;  // TODO: Extract from postsolve storage
        result->presolve_time = 0.0;  // TODO: Extract timing information
        
        return result.release();
    } catch (...) {
        return nullptr;
    }
}

void papilo_result_free(papilo_result_t* result) {
    delete result;
}

papilo_status_t papilo_result_get_status(const papilo_result_t* result) {
    if (!result) {
        return PAPILO_STATUS_ERROR;
    }
    return result->status;
}

int papilo_result_get_nrows(const papilo_result_t* result) {
    if (!result || !result->presolved_problem) {
        return -1;
    }
    return result->presolved_problem->getNRows();
}

int papilo_result_get_ncols(const papilo_result_t* result) {
    if (!result || !result->presolved_problem) {
        return -1;
    }
    return result->presolved_problem->getNCols();
}

int papilo_result_get_nnz(const papilo_result_t* result) {
    if (!result || !result->presolved_problem) {
        return -1;
    }
    return result->presolved_problem->getConstraintMatrix().getNnz();
}

int papilo_result_get_objective(const papilo_result_t* result, double* coefficients, double* offset) {
    if (!result || !result->presolved_problem) {
        return PAPILO_ERROR_INVALID_PARAMETER;
    }
    
    const auto& obj = result->presolved_problem->getObjective();
    if (coefficients) {
        const auto& coeffs = obj.coefficients;
        std::copy(coeffs.begin(), coeffs.end(), coefficients);
    }
    
    if (offset) {
        *offset = obj.offset;
    }
    
    return PAPILO_OK;
}

int papilo_result_get_col_bounds(const papilo_result_t* result, double* lb, double* ub) {
    if (!result || !result->presolved_problem || (!lb && !ub)) {
        return PAPILO_ERROR_INVALID_PARAMETER;
    }
    
    const auto& lower = result->presolved_problem->getLowerBounds();
    const auto& upper = result->presolved_problem->getUpperBounds();
    
    if (lb) {
        std::copy(lower.begin(), lower.end(), lb);
    }
    
    if (ub) {
        std::copy(upper.begin(), upper.end(), ub);
    }
    
    return PAPILO_OK;
}

int papilo_result_get_row_bounds(const papilo_result_t* result, double* lhs, double* rhs) {
    if (!result || !result->presolved_problem || (!lhs && !rhs)) {
        return PAPILO_ERROR_INVALID_PARAMETER;
    }
    
    const auto& lhs_vec = result->presolved_problem->getConstraintMatrix().getLeftHandSides();
    const auto& rhs_vec = result->presolved_problem->getConstraintMatrix().getRightHandSides();
    
    if (lhs) {
        std::copy(lhs_vec.begin(), lhs_vec.end(), lhs);
    }
    
    if (rhs) {
        std::copy(rhs_vec.begin(), rhs_vec.end(), rhs);
    }
    
    return PAPILO_OK;
}

int papilo_result_get_matrix(const papilo_result_t* result, int* rows, int* cols, double* values) {
    if (!result || !result->presolved_problem || (!rows && !cols && !values)) {
        return PAPILO_ERROR_INVALID_PARAMETER;
    }
    
    const auto& matrix = result->presolved_problem->getConstraintMatrix();
    
    // Iterate through the sparse matrix structure
    int idx = 0;
    for (int col = 0; col < matrix.getNCols(); ++col) {
        auto col_range = matrix.getColumnCoefficients(col);
        for (int i = 0; i < col_range.getLength(); ++i) {
            if (rows) rows[idx] = col_range.getIndices()[i];
            if (cols) cols[idx] = col;
            if (values) values[idx] = col_range.getValues()[i];
            ++idx;
        }
    }
    
    return PAPILO_OK;
}

int papilo_result_get_num_deletions(const papilo_result_t* result, int* deleted_cols, int* deleted_rows) {
    if (!result) {
        return PAPILO_ERROR_INVALID_PARAMETER;
    }
    
    if (deleted_cols) {
        *deleted_cols = result->deleted_cols;
    }
    
    if (deleted_rows) {
        *deleted_rows = result->deleted_rows;
    }
    
    return PAPILO_OK;
}

int papilo_result_get_num_fixings(const papilo_result_t* result, int* fixed_cols) {
    if (!result) {
        return PAPILO_ERROR_INVALID_PARAMETER;
    }
    
    if (fixed_cols) {
        *fixed_cols = result->fixed_cols;
    }
    
    return PAPILO_OK;
}

double papilo_result_get_presolve_time(const papilo_result_t* result) {
    if (!result) {
        return -1.0;
    }
    return result->presolve_time;
}

} // extern "C"