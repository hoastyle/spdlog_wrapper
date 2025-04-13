#!/bin/bash
# Run script for MM-Logger Performance Analyzer
# This script simplifies running the analyzer with proper settings

# Color definitions
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Default settings
OUTPUT_DIR="./perf_report"
FIX_FONTS=false
DEBUG_MODE=false
NO_UNICODE=false

# Show help message
show_help() {
  echo "MM-Logger Performance Analysis Tool"
  echo "Usage: $0 [options] <results_csv_file>"
  echo
  echo "Options:"
  echo "  -o, --output DIR    Output directory (default: ./perf_report)"
  echo "  -f, --fix-fonts     Fix font-related issues before running"
  echo "  -d, --debug         Enable debug mode (show error traces)"
  echo "  -n, --no-unicode    Avoid using Unicode characters in outputs"
  echo "  -h, --help          Show this help message"
  echo
  echo "Example:"
  echo "  $0 --fix-fonts --output ./my_report mm_logger_perf_results.csv"
  echo
}

# Parse command line arguments
POSITIONAL=()
while [[ $# -gt 0 ]]; do
  key="$1"
  case $key in
    -o|--output)
      OUTPUT_DIR="$2"
      shift
      shift
      ;;
    -f|--fix-fonts)
      FIX_FONTS=true
      shift
      ;;
    -d|--debug)
      DEBUG_MODE=true
      shift
      ;;
    -n|--no-unicode)
      NO_UNICODE=true
      shift
      ;;
    -h|--help)
      show_help
      exit 0
      ;;
    *)
      POSITIONAL+=("$1")
      shift
      ;;
  esac
done

# Restore positional parameters
set -- "${POSITIONAL[@]}"

# Check if CSV file is provided
if [ $# -eq 0 ]; then
  echo -e "${RED}Error: No CSV file specified${NC}"
  show_help
  exit 1
fi

CSV_FILE="$1"

# Check if CSV file exists
if [ ! -f "$CSV_FILE" ]; then
  echo -e "${RED}Error: CSV file not found: $CSV_FILE${NC}"
  exit 1
fi

# Prepare command line options
OPTIONS=""

if [ "$DEBUG_MODE" = true ]; then
  OPTIONS="$OPTIONS --debug"
fi

if [ "$NO_UNICODE" = true ]; then
  OPTIONS="$OPTIONS --no-unicode"
fi

# Fix fonts if requested
if [ "$FIX_FONTS" = true ]; then
  echo -e "${BLUE}Fixing font configuration...${NC}"
  python fix_fonts.py
  
  if [ $? -ne 0 ]; then
    echo -e "${YELLOW}Warning: Font configuration may not be complete${NC}"
  fi
fi

# Run the analyzer
echo -e "${GREEN}Running MM-Logger Performance Analyzer...${NC}"
echo -e "${BLUE}CSV File: $CSV_FILE${NC}"
echo -e "${BLUE}Output Directory: $OUTPUT_DIR${NC}"

python analyze_perf_results.py $OPTIONS "$CSV_FILE" "$OUTPUT_DIR"

# Check exit status
EXIT_CODE=$?
if [ $EXIT_CODE -eq 0 ]; then
  echo -e "${GREEN}Analysis completed successfully!${NC}"
  echo -e "Results saved to: ${BLUE}$OUTPUT_DIR${NC}"
  
  # Show main report file
  REPORT_FILE="$OUTPUT_DIR/performance_summary_report.md"
  if [ -f "$REPORT_FILE" ]; then
    echo -e "Main report: ${BLUE}$REPORT_FILE${NC}"
  fi
else
  echo -e "${RED}Analysis failed with exit code: $EXIT_CODE${NC}"
  echo -e "Check the error messages above for details."
fi

exit $EXIT_CODE
