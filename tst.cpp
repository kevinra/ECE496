#include <iostream>

#ifdef DEBUG
  #define DBG_OUT_VAR(var) \
    std::cout << "DBG: " << __FILE__ << "(" << __LINE__ << ")::"\
         << __func__ << "> " << #var << " = [" << (var) << "]" << std::endl
  #define DBG_ERR_VAR(var) \
    std::cerr << "DBG: " << __FILE__ << "(" << __LINE__ << ") "\
         << #var << " = [" << (var) << "]" << std::endl
  #define DBG_OUT_MSG(msg) \
    std::cout << "DBG: " << __FILE__ << "(" << __LINE__ << ") " \
         << msg << std::endl
  #define DBG_ERR_MSG(msg) \
    std::cerr << "DBG: " << __FILE__ << "(" << __LINE__ << ") " \
         << msg << std::endl
#else
  #define DBG_OUT_VAR(var)
  #define DBG_ERR_VAR(var)
  #define DBG_OUT_MSG(msg)
  #define DBG_ERR_MSG(msg)
#endif

double abs(double a)
{
  if (a < 0)
  	return a * -1;
  return a;
}


int main()
{
	int t, counter = 0;
	int vals[20] = {20, 20, 30, 30, 30, 30, 22, 25, 50, 50, 50, 50, 50, 50, 51, 49, 20, 20, 20, 34};
	double alp = 0.875, beta = 0.75, t_rtt_old = 20, t_rtt_new, d_rtt_old = 10, d_rtt_new, rtt;

  while (counter < 20)
  {
  	std::cout << counter << " rep" << std::endl;
		// std::cin >> t;
		t = vals[counter];
		std::cout << "t = " << t << std::endl;

		t_rtt_new = alp * t_rtt_old + (1 - alp) * t;
		d_rtt_new = beta * d_rtt_old + (1 - beta) * abs(t - t_rtt_old);
		// rtt = t_rtt_new + d_rtt_new;
		// rtt = t_rtt_new;
	  t_rtt_old = t_rtt_new;
	  // d_rtt_old = d_rtt_new;

	  // std::cout << "T(RTT)_new = " << t_rtt_new << " d(RTT)_new = " << d_rtt_new << " RT = " << rtt << std::endl;
	  std::cout << "T(RTT)_new = " << t_rtt_new << std::endl;
    counter++;
  }

  int sum = 0;
  for (int i = 0; i < 20; i++)
  {
  	sum += vals[i];
  }

  std::cout << "Average = " << sum / 20 << std::endl;

	return 0;
}

