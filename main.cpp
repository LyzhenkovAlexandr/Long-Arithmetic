#include "LN.h"
#include "return_codes.h"

template< typename FN >
void ExecuteBinary(std::stack< LN > &state, FN &&func)
{
	LN n1 = state.top();
	state.pop();
	LN n2 = state.top();
	state.pop();
	state.emplace(func(n1, n2));
}

template< typename FN >
void ExecuteUnary(std::stack< LN > &state, FN &&func)
{
	LN n = state.top();
	state.pop();
	state.emplace(func(n));
}

int main(int argc, char **argv)
{
	if (argc != 3)
	{
		std::cerr << "Number of parameters is incorrect" << std::endl;
		return ERROR_PARAMETER_INVALID;
	}
	try
	{
		std::ifstream in(argv[1]);
		if (in.bad() || in.fail())
		{
			std::cerr << "Error has occurred on ifstream" << std::endl;
			return ERROR_CANNOT_OPEN_FILE;
		}
		std::string element;
		std::stack< LN > numbers;
		while (in >> element)
		{
			if (element == "+")
			{
				ExecuteBinary(numbers, [](const auto &n1, const auto &n2) { return n1 + n2; });
			}
			else if (element == "-")
			{
				ExecuteBinary(numbers, [](const auto &n1, const auto &n2) { return n1 - n2; });
			}
			else if (element == "/")
			{
				ExecuteBinary(numbers, [](const auto &n1, const auto &n2) { return n1 / n2; });
			}
			else if (element == "*")
			{
				ExecuteBinary(numbers, [](const auto &n1, const auto &n2) { return n1 * n2; });
			}
			else if (element == "%")
			{
				ExecuteBinary(numbers, [](const auto &n1, const auto &n2) { return n1 % n2; });
			}
			else if (element == "!=")
			{
				ExecuteBinary(numbers, [](const auto &n1, const auto &n2) { return n1 != n2; });
			}
			else if (element == "==")
			{
				ExecuteBinary(numbers, [](const auto &n1, const auto &n2) { return n1 == n2; });
			}
			else if (element == ">=")
			{
				ExecuteBinary(numbers, [](const auto &n1, const auto &n2) { return n1 >= n2; });
			}
			else if (element == "<=")
			{
				ExecuteBinary(numbers, [](const auto &n1, const auto &n2) { return n1 <= n2; });
			}
			else if (element == "<")
			{
				ExecuteBinary(numbers, [](const auto &n1, const auto &n2) { return n1 < n2; });
			}
			else if (element == ">")
			{
				ExecuteBinary(numbers, [](const auto &n1, const auto &n2) { return n1 > n2; });
			}
			else if (element == "_")
			{
				ExecuteUnary(numbers, [](const auto &n) { return -n; });
			}
			else if (element == "~")
			{
				ExecuteUnary(numbers, [](const auto &n) { return ~n; });
			}
			else
			{
				numbers.emplace(element);
				if (numbers.top().IsNaN())
				{
					std::cerr << "Invalid operation with NaN" << std::endl;
					return ERROR_DATA_INVALID;
				}
			}
		}

		std::ofstream out(argv[2]);
		if (out.bad() || out.fail())
		{
			std::cerr << "Error has occurred on ofstream" << std::endl;
			return ERROR_CANNOT_OPEN_FILE;
		}
		while (!numbers.empty())
		{
			out << numbers.top().ToString() << std::endl;
			numbers.pop();
		}
	} catch (const std::bad_alloc &)
	{
		std::cerr << "Error with memory allocation" << std::endl;
		return ERROR_OUT_OF_MEMORY;
	} catch (const std::domain_error &ex)
	{
		std::cerr << ex.what() << std::endl;
		return ERROR_DATA_INVALID;
	} catch (...)
	{
		std::cerr << "Unknown error" << std::endl;
		return ERROR_UNKNOWN;
	}

	return SUCCESS;
}
