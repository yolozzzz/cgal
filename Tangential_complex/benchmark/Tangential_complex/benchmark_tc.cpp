//#undef CGAL_LINKED_WITH_TBB // CJTODO TEMP

// Without TBB_USE_THREADING_TOOL Intel Inspector XE will report false positives in Intel TBB
// (http://software.intel.com/en-us/articles/compiler-settings-for-threading-error-analysis-in-intel-inspector-xe/)
#ifdef _DEBUG
# define TBB_USE_THREADING_TOOL
#endif

#include <CGAL/assertions_behaviour.h>
#include <CGAL/Epick_d.h>
#include <CGAL/Tangential_complex.h>
#include <CGAL/Random.h>
#include <CGAL/Mesh_3/Profiling_tools.h>

#include "../../test/Tangential_complex/test_utilities.h"

#include <boost/algorithm/string/replace.hpp>
#include <boost/algorithm/string/trim_all.hpp>

#include <cstdlib>
#include <fstream>
#include <math.h>

#ifdef CGAL_LINKED_WITH_TBB
# include <tbb/task_scheduler_init.h>
#endif
#include "XML_exporter.h"
#define CGAL_TC_EXPORT_PERFORMANCE_DATA
#define CGAL_TC_SET_PERFORMANCE_DATA(value_name, value) \
        XML_perf_data::set(value_name, value);

const char * const BENCHMARK_SCRIPT_FILENAME = "benchmark_script.txt";

typedef CGAL::Epick_d<CGAL::Dynamic_dimension_tag>              Kernel;
typedef Kernel::FT                                              FT;
typedef Kernel::Point_d                                         Point;
typedef CGAL::Tangential_complex<
  Kernel, CGAL::Dynamic_dimension_tag,
  CGAL::Parallel_tag>                                           TC;

class XML_perf_data
{
public:
  typedef Streaming_XML_exporter<std::string> XML_exporter;

  XML_perf_data(const std::string &filename)
    : m_xml(filename, "ContainerPerformance", "Perf",
            construct_subelements_names())
  {}

  virtual ~XML_perf_data()
  {
  }

  static XML_perf_data &get()
  {
    static XML_perf_data singleton(build_filename());
    return singleton;
  }

  template <typename Value_type>
  static void set(const std::string &name, Value_type value)
  {
    get().set_data(name, value);
  }

  static void commit()
  {
    get().commit_current_element();
  }

protected:
  static std::string build_filename()
  {
    std::stringstream sstr;
    sstr << "perf_logs/Performance_log_" << time(0) << ".xml";
    return sstr.str();
  }

  static std::vector<std::string> construct_subelements_names()
  {
    std::vector<std::string> subelements;
    subelements.push_back("Input");
    subelements.push_back("Intrinsic_dim");
    subelements.push_back("Ambient_dim");
    subelements.push_back("Sparsity");
    subelements.push_back("Num_points_in_input");
    subelements.push_back("Num_points");
    subelements.push_back("Initial_num_inconsistent_local_tr");
    subelements.push_back("Best_num_inconsistent_local_tr");
    subelements.push_back("Final_num_inconsistent_local_tr");
    subelements.push_back("Init_time");
    subelements.push_back("Comput_time");
    subelements.push_back("Perturb_successful");
    subelements.push_back("Perturb_time");
    subelements.push_back("Perturb_steps");
    subelements.push_back("Add_higher_dim_simpl_time");
    subelements.push_back("Result_pure_pseudomanifold");
    subelements.push_back("Result_num_wrong_dim_simplices");
    subelements.push_back("Result_num_wrong_number_of_cofaces");
    subelements.push_back("Result_num_unconnected_stars");
    subelements.push_back("Info");

    return subelements;
  }

  void set_data(const std::string &name, const std::string &value)
  {
    m_current_element[name] = value;
  }

  template <typename Value_type>
  void set_data(const std::string &name, Value_type value)
  {
    std::stringstream sstr;
    sstr << value;
    set_data(name, sstr.str());
  }

  void commit_current_element()
  {
    m_xml.add_element(m_current_element);
    m_current_element.clear();
  }

  XML_exporter m_xml;
  XML_exporter::Element_with_map m_current_element;
};

class Test_dim
{
public:
  Test_dim(
    int min_allowed_dim = 0, 
    int max_allowed_dim = std::numeric_limits<int>::max())
    : m_min_allowed_dim(min_allowed_dim), m_max_allowed_dim(max_allowed_dim)
  {}

  template <typename Simplex>
  bool operator()(Simplex const& s)
  {
    return s.size() - 1 >= m_min_allowed_dim
      && s.size() - 1 <= m_max_allowed_dim;
  }

private:
  int m_min_allowed_dim;
  int m_max_allowed_dim;
};

template <typename TC>
bool export_to_off(
  TC const& tc, 
  std::string const& input_name_stripped,
  std::string const& suffix,
  bool color_inconsistencies = false,
  typename TC::Simplicial_complex const* p_complex = NULL,
  std::set<std::set<std::size_t> > const *p_simpl_to_color_in_red = NULL,
  std::set<std::set<std::size_t> > const *p_simpl_to_color_in_green = NULL,
  std::set<std::set<std::size_t> > const *p_simpl_to_color_in_blue = NULL)
{
  if (tc.intrinsic_dimension() <= 3)
  {
    std::stringstream output_filename;
    output_filename << "output/" << input_name_stripped << "_" 
      << tc.intrinsic_dimension() << "_in_R" 
      << tc.ambient_dimension() << suffix << ".off";
    std::ofstream off_stream(output_filename.str().c_str());

    if (p_complex)
    {
      tc.export_to_off(
        *p_complex, off_stream, 
        p_simpl_to_color_in_red,
        p_simpl_to_color_in_green, 
        p_simpl_to_color_in_blue);
    }
    else
    {
#ifdef CGAL_ALPHA_TC
      TC::Simplicial_complex complex;
      tc.export_TC(complex, false);
      tc.export_to_off(
        complex, off_stream, 
        p_simpl_to_color_in_red,
        p_simpl_to_color_in_green, 
        p_simpl_to_color_in_blue);
#else
      tc.export_to_off(
        off_stream, color_inconsistencies, 
        p_simpl_to_color_in_red,
        p_simpl_to_color_in_green, 
        p_simpl_to_color_in_blue);
#endif
    }
    return true;
  }
  return false;
}

void make_tc(std::vector<Point> &points, 
             int intrinsic_dim,
             double sparsity = 0., 
             bool perturb = true, 
             bool add_high_dim_simpl = false, 
             bool collapse = false,
             double time_limit_for_perturb = 0.,
             const char *input_name = "tc")
{
  // CJTODO TEMP TEST
  //TC::Simplicial_complex compl;
  //{std::size_t ss[] = {0, 1, 2}; compl.add_simplex(std::set<std::size_t>(ss, ss + 3)); }
  //{std::size_t ss[] = {0, 2, 3}; compl.add_simplex(std::set<std::size_t>(ss, ss + 3)); }
  //{std::size_t ss[] = {0, 3, 4}; compl.add_simplex(std::set<std::size_t>(ss, ss + 3)); }
  //{std::size_t ss[] = {0, 4, 1}; compl.add_simplex(std::set<std::size_t>(ss, ss + 3)); }
  //{std::size_t ss[] = {0, 5, 6}; compl.add_simplex(std::set<std::size_t>(ss, ss + 3)); }
  //compl.is_pure_pseudomanifold(2, 7, false, 10);

  //TC::Simplicial_complex compl;
  //{std::size_t ss[] = {0, 1, 2, 5}; compl.add_simplex(std::set<std::size_t>(ss, ss + 4)); }
  //{std::size_t ss[] = {0, 2, 3, 5}; compl.add_simplex(std::set<std::size_t>(ss, ss + 4)); }
  //{std::size_t ss[] = {0, 3, 4, 5}; compl.add_simplex(std::set<std::size_t>(ss, ss + 4)); }
  //{std::size_t ss[] = {0, 4, 1, 5}; compl.add_simplex(std::set<std::size_t>(ss, ss + 4)); }
  //{std::size_t ss[] = {0, 1, 2, 6}; compl.add_simplex(std::set<std::size_t>(ss, ss + 4)); }
  //{std::size_t ss[] = {0, 2, 3, 6}; compl.add_simplex(std::set<std::size_t>(ss, ss + 4)); }
  //{std::size_t ss[] = {0, 3, 4, 6}; compl.add_simplex(std::set<std::size_t>(ss, ss + 4)); }
  //{std::size_t ss[] = {0, 4, 1, 6}; compl.add_simplex(std::set<std::size_t>(ss, ss + 4)); }
  //{std::size_t ss[] = {0, 4, 7, 8}; compl.add_simplex(std::set<std::size_t>(ss, ss + 4)); }
  //compl.is_pure_pseudomanifold(3, 9, false, 10);
  // /CJTODO TEMP TEST

  //===========================================================================
  // Init
  //===========================================================================
  Kernel k;
  Wall_clock_timer t;

  // Get input_name_stripped
  std::string input_name_stripped(input_name);
  size_t slash_index = input_name_stripped.find_last_of('/');
  if (slash_index == std::string::npos)
    slash_index = input_name_stripped.find_last_of('\\');
  if (slash_index == std::string::npos)
    slash_index = 0;
  else
    ++slash_index;
  input_name_stripped = input_name_stripped.substr(
    slash_index, input_name_stripped.find_last_of('.') - slash_index);

  int ambient_dim = k.point_dimension_d_object()(*points.begin());

#ifdef CGAL_TC_PROFILING
  Wall_clock_timer t_gen;
#endif

#ifdef CGAL_TC_PROFILING
  std::cerr << "Point set generated in " << t_gen.elapsed()
            << " seconds." << std::endl;
#endif

  CGAL_TC_SET_PERFORMANCE_DATA("Num_points_in_input", points.size());

#ifdef USE_ANOTHER_POINT_SET_FOR_TANGENT_SPACE_ESTIM
  std::vector<Point> points_not_sparse = points;
#endif

  //===========================================================================
  // Sparsify point set if requested
  //===========================================================================
  if (sparsity != 0.)
  {
    std::size_t num_points_before = points.size();
    points = sparsify_point_set(k, points, sparsity*sparsity);
    std::cerr << "Number of points before/after sparsification: "
      << num_points_before << " / " << points.size() << std::endl;
  }

  CGAL_TC_SET_PERFORMANCE_DATA("Sparsity", sparsity);
  CGAL_TC_SET_PERFORMANCE_DATA("Num_points", points.size());

  //===========================================================================
  // Compute Tangential Complex
  //===========================================================================

#ifdef USE_ANOTHER_POINT_SET_FOR_TANGENT_SPACE_ESTIM
  TC tc(points.begin(), points.end(), sparsity, intrinsic_dim,
    points_not_sparse.begin(), points_not_sparse.end(), k);
#else
  TC tc(points.begin(), points.end(), sparsity, intrinsic_dim, k);
#endif

  double init_time = t.elapsed(); t.reset();

  tc.compute_tangential_complex();
  double computation_time = t.elapsed(); t.reset();

  //===========================================================================
  // CJTODO TEMP
  //===========================================================================
  /*{
  TC::Simplicial_complex complex;
  int max_dim = tc.export_TC(complex, false);
  complex.display_stats();

  std::stringstream output_filename;
  output_filename << "output/" << input_name_stripped << "_" << intrinsic_dim
    << "_in_R" << ambient_dim << "_ALPHA_COMPLEX.off";
  std::ofstream off_stream(output_filename.str().c_str());
  tc.export_to_off(complex, off_stream);

  // Collapse
  complex.collapse(max_dim);
  {
  std::stringstream output_filename;
  output_filename << "output/" << input_name_stripped << "_" << intrinsic_dim
    << "_in_R" << ambient_dim << "_AFTER_COLLAPSE.off";
  std::ofstream off_stream(output_filename.str().c_str());
  tc.export_to_off(complex, off_stream);
  }
  std::size_t num_wrong_dim_simplices, 
              num_wrong_number_of_cofaces, 
              num_unconnected_stars;
  bool pure_manifold = complex.is_pure_pseudomanifold(
    intrinsic_dim, tc.number_of_vertices(), false, 1,
    &num_wrong_dim_simplices, &num_wrong_number_of_cofaces, 
    &num_unconnected_stars);
  complex.display_stats();
  }
  return;*/
  // CJTODO TEMP ===========================

  //tc.check_if_all_simplices_are_in_the_ambient_delaunay();

  //===========================================================================
  // Export to OFF
  //===========================================================================
  t.reset();
  double export_before_time = 
    (export_to_off(tc, input_name_stripped, "_BEFORE_FIX") ? t.elapsed() : -1);
  t.reset();
  
  unsigned int num_perturb_steps = 0;
  double perturb_time = -1;
    double export_after_perturb_time = -1.;
  CGAL::Fix_inconsistencies_status perturb_ret = CGAL::FIX_NOT_PERFORMED;
  if (perturb)
  {
    //=========================================================================
    // Try to fix inconsistencies by perturbing points
    //=========================================================================
    t.reset();
    std::size_t initial_num_inconsistent_local_tr;
    std::size_t best_num_inconsistent_local_tr;
    std::size_t final_num_inconsistent_local_tr;
    perturb_ret = tc.fix_inconsistencies(
      num_perturb_steps, initial_num_inconsistent_local_tr,
      best_num_inconsistent_local_tr, final_num_inconsistent_local_tr,
      time_limit_for_perturb);
    perturb_time = t.elapsed(); t.reset();

    CGAL_TC_SET_PERFORMANCE_DATA("Initial_num_inconsistent_local_tr", 
                                 initial_num_inconsistent_local_tr);
    CGAL_TC_SET_PERFORMANCE_DATA("Best_num_inconsistent_local_tr", 
                                 best_num_inconsistent_local_tr);
    CGAL_TC_SET_PERFORMANCE_DATA("Final_num_inconsistent_local_tr", 
                                 final_num_inconsistent_local_tr);

    //=========================================================================
    // Export to OFF
    //=========================================================================
    t.reset();
    bool exported = export_to_off(tc, input_name_stripped, "_AFTER_FIX", true);
    double export_after_perturb_time = (exported ? t.elapsed() : -1);
    t.reset();
  }
  else
  {
    CGAL_TC_SET_PERFORMANCE_DATA("Initial_num_inconsistent_local_tr", "N/A");
    CGAL_TC_SET_PERFORMANCE_DATA("Best_num_inconsistent_local_tr", "N/A");
    CGAL_TC_SET_PERFORMANCE_DATA("Final_num_inconsistent_local_tr", "N/A");
  }

  int max_dim = -1;
  double fix2_time = -1;
  double export_after_fix2_time = -1.;
  TC::Simplicial_complex complex;
  if (add_high_dim_simpl)
  {
    //=========================================================================
    // Try to fix inconsistencies by adding higher-dimension simplices
    //=========================================================================
    t.reset();
    // Try to solve the remaining inconstencies
    tc.check_and_solve_inconsistencies_by_adding_higher_dim_simplices();
    fix2_time = t.elapsed(); t.reset();
    max_dim = tc.export_TC(complex, false);
    /*std::set<std::set<std::size_t> > not_delaunay_simplices;
    if (ambient_dim <= 4)
    {
      tc.check_if_all_simplices_are_in_the_ambient_delaunay(
        &complex, true, &not_delaunay_simplices);
    }*/
  
    //=========================================================================
    // Export to OFF
    //=========================================================================
    t.reset();
    bool exported = export_to_off(
      tc, input_name_stripped, "_AFTER_FIX2", false, &complex);
    double export_after_fix2_time = (exported ? t.elapsed() : -1);
    t.reset();
  }
  else
  {
    max_dim = tc.export_TC(complex, false);
  }

  complex.display_stats();

  // Export to OFF with higher-dim simplices colored
  std::set<std::set<std::size_t> > higher_dim_simplices;
  complex.get_simplices_matching_test(
    Test_dim(intrinsic_dim + 1),
    std::inserter(higher_dim_simplices, higher_dim_simplices.begin()));
  export_to_off(
    tc, input_name_stripped, "_BEFORE_COLLAPSE", false, &complex, 
    &higher_dim_simplices);
  
  //===========================================================================
  // Collapse
  //===========================================================================
  if (collapse)
    complex.collapse(max_dim);

  //===========================================================================
  // Is the result a pure pseudomanifold?
  //===========================================================================
  std::size_t num_wrong_dim_simplices, 
              num_wrong_number_of_cofaces, 
              num_unconnected_stars;
  std::set<std::set<std::size_t> > wrong_dim_simplices;
  std::set<std::set<std::size_t> > wrong_number_of_cofaces_simplices;
  std::set<std::set<std::size_t> > unconnected_stars_simplices;
  bool is_pure_pseudomanifold = complex.is_pure_pseudomanifold(
    intrinsic_dim, tc.number_of_vertices(), false, 1,
    &num_wrong_dim_simplices, &num_wrong_number_of_cofaces, 
    &num_unconnected_stars,
    &wrong_dim_simplices, &wrong_number_of_cofaces_simplices, 
    &unconnected_stars_simplices);

  // Stats about the simplices
  complex.display_stats();

  //===========================================================================
  // Export to OFF
  //===========================================================================
  t.reset();
  bool exported = export_to_off(
    tc, input_name_stripped, "_AFTER_COLLAPSE", false, &complex, 
    &wrong_dim_simplices, &wrong_number_of_cofaces_simplices, 
    &unconnected_stars_simplices);
  std::cerr 
    << " OFF colors:" << std::endl
    << "   * Red: wrong dim simplices" << std::endl
    << "   * Green: wrong number of cofaces simplices" << std::endl
    << "   * Blue: not-connected stars" << std::endl;
  double export_after_collapse_time = (exported ? t.elapsed() : -1);
  t.reset();

  //===========================================================================
  // Display info
  //===========================================================================

  std::cerr << std::endl
    << "================================================" << std::endl
    << "Number of vertices: " << tc.number_of_vertices() << std::endl
    << "Pure pseudomanifold: " << (is_pure_pseudomanifold ? "YES" : "NO") << std::endl
    << "Computation times (seconds): " << std::endl
    << "  * Tangential complex: " << init_time + computation_time << std::endl
    << "    - Init + kd-tree = " << init_time << std::endl
    << "    - TC computation = " << computation_time << std::endl
    << "  * Export to OFF (before perturb): " << export_before_time << std::endl
    << "  * Fix inconsistencies 1: " << perturb_time
    <<      " (" << num_perturb_steps << " steps) ==> "
    <<      (perturb_ret == CGAL::TC_FIXED ? "FIXED" : "NOT fixed") << std::endl
    << "  * Fix inconsistencies 2: " << fix2_time << std::endl
    << "  * Export to OFF (after perturb): " << export_after_perturb_time << std::endl
    << "  * Export to OFF (after fix2): "<< export_after_fix2_time << std::endl
    << "  * Export to OFF (after collapse): "
    <<      export_after_collapse_time << std::endl
    << "================================================" << std::endl
    << std::endl;
  
  //===========================================================================
  // Export info
  //===========================================================================
  CGAL_TC_SET_PERFORMANCE_DATA("Init_time", init_time);
  CGAL_TC_SET_PERFORMANCE_DATA("Comput_time", computation_time);
  CGAL_TC_SET_PERFORMANCE_DATA("Perturb_successful",
                                (perturb_ret == CGAL::TC_FIXED ? "Y" : "N"));
  CGAL_TC_SET_PERFORMANCE_DATA("Perturb_time", perturb_time);
  CGAL_TC_SET_PERFORMANCE_DATA("Perturb_steps", num_perturb_steps);
  CGAL_TC_SET_PERFORMANCE_DATA("Add_higher_dim_simpl_time", fix2_time);
  CGAL_TC_SET_PERFORMANCE_DATA("Result_pure_pseudomanifold",
                                (is_pure_pseudomanifold ? "Y" : "N"));
  CGAL_TC_SET_PERFORMANCE_DATA("Result_num_wrong_dim_simplices",
                                num_wrong_dim_simplices);
  CGAL_TC_SET_PERFORMANCE_DATA("Result_num_wrong_number_of_cofaces", 
                                num_wrong_number_of_cofaces);
  CGAL_TC_SET_PERFORMANCE_DATA("Result_num_unconnected_stars", 
                                num_unconnected_stars);
  CGAL_TC_SET_PERFORMANCE_DATA("Info", "");
}

int main()
{
#ifdef CGAL_LINKED_WITH_TBB
# ifdef _DEBUG
  int num_threads = 1;
# else
  int num_threads = 10;
# endif
#endif

  int seed = CGAL::default_random.get_int(0, 1<<30);
  CGAL::default_random = CGAL::Random();
  std::cerr << "Random seed = " << seed << std::endl;

  std::ifstream script_file;
  script_file.open(BENCHMARK_SCRIPT_FILENAME);
  // Script?
  // Script file format: each line gives
  //    - Filename (point set) or "generate_XXX" (point set generation)
  //    - Ambient dim
  //    - Intrinsic dim
  //    - Number of iterations with these parameters
  if (script_file.is_open())
  {
    int i = 1;
#ifdef CGAL_LINKED_WITH_TBB
# ifdef BENCHMARK_WITH_1_TO_MAX_THREADS
    for(num_threads = 1 ;
          num_threads <= tbb::task_scheduler_init::default_num_threads() ;
          ++num_threads)
# endif
#endif
    /*for (Concurrent_mesher_config::get().num_work_items_per_batch = 5 ;
      Concurrent_mesher_config::get().num_work_items_per_batch < 100 ;
      Concurrent_mesher_config::get().num_work_items_per_batch += 5)*/
    {
#ifdef CGAL_LINKED_WITH_TBB
      tbb::task_scheduler_init init(
        num_threads > 0 ? num_threads : tbb::task_scheduler_init::automatic);
#endif

      std::cerr << "Script file '" << BENCHMARK_SCRIPT_FILENAME << "' found." << std::endl;
      script_file.seekg(0);
      while (script_file.good())
      {
        std::string line;
        std::getline(script_file, line);
        if (line.size() > 1 && line[0] != '#')
        {
          boost::replace_all(line, "\t", " ");
          boost::trim_all(line);
          std::cerr << std::endl << std::endl;
          std::cerr << "*****************************************" << std::endl;
          std::cerr << "******* " << line << std::endl;
          std::cerr << "*****************************************" << std::endl;
          std::stringstream sstr(line);

          std::string input;
          std::string param1;
          std::string param2;
          std::string param3;
          std::size_t num_points;
          int ambient_dim;
          int intrinsic_dim;
          double sparsity;
          char perturb, add_high_dim_simpl, collapse;
          double time_limit_for_perturb;
          int num_iteration;
          sstr >> input;
          sstr >> param1;
          sstr >> param2;
          sstr >> param3;
          sstr >> num_points;
          sstr >> ambient_dim;
          sstr >> intrinsic_dim;
          sstr >> sparsity;
          sstr >> perturb;
          sstr >> add_high_dim_simpl;
          sstr >> collapse;
          sstr >> time_limit_for_perturb;
          sstr >> num_iteration;

          for (int j = 0 ; j < num_iteration ; ++j)
          {
            std::string input_stripped = input;
            size_t slash_index = input_stripped.find_last_of('/');
            if (slash_index == std::string::npos)
              slash_index = input_stripped.find_last_of('\\');
            if (slash_index == std::string::npos)
              slash_index = 0;
            else
              ++slash_index;
            input_stripped = input_stripped.substr(
              slash_index, input_stripped.find_last_of('.') - slash_index);

            CGAL_TC_SET_PERFORMANCE_DATA("Input", input_stripped);
            CGAL_TC_SET_PERFORMANCE_DATA("Ambient_dim", ambient_dim);
            CGAL_TC_SET_PERFORMANCE_DATA("Intrinsic_dim", intrinsic_dim);
#ifdef CGAL_LINKED_WITH_TBB
            CGAL_TC_SET_PERFORMANCE_DATA(
              "Num_threads",
              (num_threads == -1 ? tbb::task_scheduler_init::default_num_threads() : num_threads));
#else
            CGAL_TC_SET_PERFORMANCE_DATA("Num_threads", "N/A");
#endif

            std::cerr << std::endl << "TC #" << i << "..." << std::endl;

            std::vector<Point> points;

            if (input == "generate_moment_curve")
            {
              points = generate_points_on_moment_curve<Kernel>(
                num_points, ambient_dim,
                std::atof(param1.c_str()), std::atof(param2.c_str()));
            }
            else if (input == "generate_plane")
            {
              points = generate_points_on_plane<Kernel>(num_points);
            }
            else if (input == "generate_sphere_d")
            {
              points = generate_points_on_sphere_d<Kernel>(
                num_points, ambient_dim,
                std::atof(param1.c_str()),
                std::atof(param2.c_str()));
            }
            else if (input == "generate_klein_bottle_3D")
            {
              points = generate_points_on_klein_bottle_3D<Kernel>(
                num_points,
                std::atof(param1.c_str()), std::atof(param2.c_str()));
            }
            else if (input == "generate_klein_bottle_4D")
            {
              points = generate_points_on_klein_bottle_4D<Kernel>(
                num_points,
                std::atof(param1.c_str()), std::atof(param2.c_str()));
            }
            else if (input == "generate_klein_bottle_variant_5D")
            {
              points = generate_points_on_klein_bottle_variant_5D<Kernel>(
                num_points,
                std::atof(param1.c_str()), std::atof(param2.c_str()));
            }
            else
            {
              load_points_from_file<Point>(
                input, std::back_inserter(points)/*, 600*/);
            }

            if (!points.empty())
            {
              make_tc(points, intrinsic_dim, sparsity,
                      perturb=='Y', add_high_dim_simpl=='Y', collapse=='Y',
                      time_limit_for_perturb, input.c_str());

              std::cerr << "TC #" << i++ << " done." << std::endl;
              std::cerr << std::endl << "---------------------------------"
                        << std::endl << std::endl;
            }
            else
            {
              std::cerr << "TC #" << i++ << ": no points loaded." << std::endl;
            }

            XML_perf_data::commit();
          }
        }
      }
      script_file.seekg(0);
      script_file.clear();
    }

    script_file.close();
  }
  // Or not script?
  else
  {
    std::cerr << "Script file '" << BENCHMARK_SCRIPT_FILENAME << "' NOT found." << std::endl;
  }

  return 0;
}
