import java.sql.Connection;
import java.sql.DriverManager;
import java.sql.ResultSet;
import java.sql.SQLException;
import java.util.*;

/**
 * Main Class for the project
 */
public class pgtest {
    /**
     * The entry of the program
     *
     * @param args not used
     */
    public static void main(String[] args) {
        String url = "jdbc:postgresql://stampy.cs.wisc.edu/cs564instr?sslfactory=org.postgresql.ssl.NonValidatingFactory&ssl";
        SQLExecutor.connect(url);
        while (true) {
            try {
                iteration();
            } catch (SQLException e) {
                System.out.println("Invalid Query");
            }
        }
    }

    /**
     * This method is called on each iteration of sampling
     *
     * @throws SQLException
     */
    private static void iteration() throws SQLException {
        Set<Integer> selected = new HashSet<>();
        String query = Prompter.userQuery();
        if (query == null){
            SQLExecutor.executeUpdate(QueryBuilder.dropTable(null));
            System.exit(0);
        }
        int N = SQLExecutor.getCount(query);

        while (true) { // sample
            int n = Prompter.sampleSize();
            if (n > N) {
                n = Math.min(n, N);
                System.out.println("The input size exceed the size limit, " + N + "results are selected");
            }
            long seed = Prompter.seed();
            String outputTableName = Prompter.outputTableName();

            Integer[] sampleRowNum = sampleNumer(N, n, seed, selected);
            N -= n;
            Collections.addAll(selected, sampleRowNum);

            SQLExecutor.executeUpdate(QueryBuilder.dropTable(outputTableName));
            SQLExecutor.executeUpdate(QueryBuilder.sampleRowIntoTable(query, sampleRowNum, outputTableName));
            SQLExecutor.executeUpdate(QueryBuilder.dropRowNum(outputTableName));

            if (outputTableName == null) { // output to console
                ResultPrinter.print(SQLExecutor.executeQuery(QueryBuilder.selectAllFromTable(outputTableName)));
            } else // save to table
                System.out.println("Results are saved into table: " + outputTableName);

            if (N <= 0) {
                System.out.println("No more samples available.");
                break;
            }
            if (!Prompter.continueSample())
                break;
        }
        printLineSeparator();
    }

    /**
     * Randomly generate some row numbers of rows that has not been sampled yet,
     * assuming row number of queries are consecutive.
     *
     * @param R        number of rows remaining in the query.
     * @param n        number of rows to be sampled.
     * @param seed     seed for the random number generator
     * @param selected a set of row numbers that have already been selected.
     * @return an array of row numbers to be sampled
     */
    private static Integer[] sampleNumer(int R, int n, long seed, Set<Integer> selected) {
        Integer[] ret = new Integer[n];
        int m = 0, t = 0, N = R + selected.size();
        Random rng = new Random(seed);
        for (int i = 1; i <= N; i++) {
            if (selected.contains(i))
                continue;
            if (rng.nextDouble() * (R - t) < n - m) {
                ret[m] = i;
                m++;
            }
            t++;
        }
        return ret;
    }

    /**
     * Print out the line seperator used between each iteration
     */
    private static void printLineSeparator() {
        System.out.println();
        System.out.println("=============================================================================");
        System.out.println();
    }
}

/**
 * The prompter prompts the user in order to ask for input Error messages are
 * printed if user's inputs are invalid
 */

class Prompter {
    private static Scanner in = new Scanner(System.in);
    private static long seed = 0;

    /**
     * Asks the user for output mode S for same in new talbe and O for output if the
     * input string has more than one character, it reads the first character as the
     * input if S is selected, the user is required to input a table name other
     * input will be treated as invalid input
     *
     * @return output table name if S is selected, or null if O is selected
     */
    static String outputTableName() {
        try {
            String line = prompt("Please select output mode (Enter S (Save in new table) / O (Output)): ");
            if (line.charAt(0) == 's') {
                String tableName = prompt("Please enter table name: ");
                if (tableName.length() == 0)
                    throw new IllegalArgumentException();

                return tableName;
            } else if (line.charAt(0) == 'o') {
                return null;
            } else {
                throw new IllegalArgumentException();
            }
        } catch (Exception e) {
            panic();
            return outputTableName();
        }
    }

    /**
     * Asks user if he/she wants to sample from a table or a query. If the input
     * starts with T, it will ask user for the table name to be sampled from. If the
     * input starts with Q, it will ask for the query to be sampled from. If the
     * input starts with E, the program will exit. This method is case insensitive.
     *
     * @return user query
     */
    static String userQuery() {
        try {
            String line = prompt("Please specify execution mode (Enter T (Table) / Q (Query) / E (Exit)): ");
            if (line.charAt(0) == 't') {
                String tableName = prompt("Please enter table name: ");
                if (tableName.length() == 0)
                    throw new IllegalArgumentException();
                return QueryBuilder.selectAllFromTable(tableName);
            } else if (line.charAt(0) == 'q') {
                return prompt("Please enter your query: ");
            } else if (line.charAt(0) == 'e') {
                return null;
            } else {
                throw new IllegalArgumentException();
            }
        } catch (Exception e) {
            panic();
            return userQuery();
        }
    }

    /**
     * Ask user for the number of samples they want for the query/table. The input
     * must be an integer that is greater than zero. If inval input is detected, the
     * user will be prompt to input a new number.
     *
     * @return the positive integer sample size
     */
    static int sampleSize() {
        try {
            String line = prompt("How many samples do you want: ");
            int sampleSize = Integer.parseInt(line);
            if (sampleSize <= 0)
                throw new IllegalArgumentException();
            return sampleSize;
        } catch (Exception e) {
            System.err.println("Sample size input must be greater than zero. Please re-enter your input");
            return sampleSize();
        }
    }

    /**
     * Ask user for the seed used for random sampling. The input cound be a long
     * integer. If N is selected, the previous seed will be used
     *
     * @return seed the seed used for random sampling
     */
    static long seed() {
        boolean setSeed = promptYesOrNo("Do you want to set seed? (Enter Y/N): ");

        if (!setSeed) {
            System.out.println("Use previous seed: " + seed);
            return seed;
        }

        while (true) {
            try {
                return seed = Long.parseLong(prompt("Please enter the seed for sampling: "));
            } catch (Exception e) {
                System.err.println("Seed must be a long integer. Please re-enter your input");
            }
        }
    }

    /**
     * Ask user whether to continue sampling
     *
     * @return whether the user wants to continue sampling
     */
    static boolean continueSample() {
        return promptYesOrNo("Do you want to continue sampling? (Enter Y/N): ");
    }

    /**
     * Promoted the message to user, and askes for Y/N input
     *
     * @param message the message printed to the console
     * @return true if the user input starts with y, or false if the user input
     *         starts with n
     */
    static boolean promptYesOrNo(String message) {
        try {
            String line = prompt(message);
            if (line.charAt(0) == 'y') {
                return true;
            } else if (line.charAt(0) == 'n') {
                return false;
            } else {
                throw new IllegalArgumentException();
            }
        } catch (Exception e) {
            panic();
            return promptYesOrNo(message);
        }
    }

    /**
     * Print the message to the console, and return the trimed user input in lower
     * case
     *
     * @param message the message to be printed to the console
     * @return
     */
    private static String prompt(String message) {
        System.out.print(message);
        return in.nextLine().toLowerCase().trim();
    }

    /**
     * Print the error message "invalid input"
     */
    private static void panic() {
        System.err.println("Invalid input.");
    }
}

/**
 * The class is used to execute SQL query
 */
class SQLExecutor {
    private static Connection conn;

    /**
     * Start a connection with the specified database.
     *
     * @param url url of the database specified.
     */
    static void connect(String url) {
        try {
            conn = DriverManager.getConnection(url);
            executeUpdate(QueryBuilder.setPath());
        } catch (SQLException e) {
            System.out.println("Cannot connect to " + url);
            System.exit(1);
        }
    }

    /**
     * Find the number of rows that will be returned by the given query.
     *
     * @param query the query to be sampled from
     * @return the number of rows of the given query
     * @throws SQLException
     */
    static Integer getCount(String query) throws SQLException {
        ResultSet rs = executeQuery(QueryBuilder.getCount(query));
        rs.next();
        return rs.getInt(1);
    }

    /**
     * Execute a query and returns the result set
     *
     * @param query the query to be executed
     * @return the result set for the query
     * @throws SQLException
     */
    static ResultSet executeQuery(String query) throws SQLException {
        // System.out.println(query);
        return conn.createStatement().executeQuery(query);
    }

    /**
     * Execute a query that updates the table (no return value)
     *
     * @param query the query to be executed
     * @return either (1) the row count for SQL Data Manipulation Language (DML)
     *         statements or (2) 0 for SQL statements that return nothing
     * @throws SQLException
     */
    static int executeUpdate(String query) throws SQLException {
        // System.out.println(query);
        return conn.createStatement().executeUpdate(query);
    }
}

/**
 * The class is used to generate a SQL query in String
 */
class QueryBuilder {
    private static final String TMP_TABLE_NAME = "panujxvbft";

    /**
     * Generate a query to set search path
     *
     * @return query to set search path to public, hw2
     */
    static String setPath() {
        return "set search_path to public, hw2;";
    }

    /**
     * Generate a query which drops the column called "rownum" in the given table
     *
     * @param tableName the name of table to be altered
     * @return query that alter the table
     */
    static String dropRowNum(String tableName) {
        return "alter table " + preprocessTableName(tableName) + " drop column rownum;";
    }

    /**
     * Generate a query which drops the table of given table name
     *
     * @param tableName the name of table to be droped
     * @return query that drops the table of given name
     */
    static String dropTable(String tableName) {
        return "drop table if exists " + preprocessTableName(tableName) + ";";
    }

    /**
     * Generate a query that selects everything from the given table
     *
     * @param tableName the name of table to be selected from
     * @return query that selects everything from the given table
     */
    static String selectAllFromTable(String tableName) {
        return "select * from " + preprocessTableName(tableName) + ";";
    }

    /**
     * Generate a query to select sample rows of input row number by using the input
     * query, and insert them into a new table named as the input table name
     *
     * @param query        Query to run
     * @param sampleRowNum Number of samples to select
     * @param tableName    The new table name
     * @return Generated query to select row and save into a new table
     */
    static String sampleRowIntoTable(String query, Integer[] sampleRowNum, String tableName) {
        return "select * into " + preprocessTableName(tableName)
                + " from ( select row_Number() over () as rownum, * from (" + truncateSemicolon(query)
                + ") s1 ) s2 where s2.rownum in (" + IntArrayToString(sampleRowNum) + ");";
    }

    /**
     * Generate a query that gets the number of rows in the table returned by the
     * query
     *
     * @param query the query that genrate the result
     * @return the number of rows in the result table returned by the query
     */
    static String getCount(String query) {
        return "select count(*) from (" + truncateSemicolon(query) + ") orig;";
    }

    /**
     * This method checks if a table name is null.
     *
     * @param tableName the table name to be checked
     * @return if tableName is not null, the input tableName will be returned.
     *         Otherwise a temperate table name will be returned.
     */
    private static String preprocessTableName(String tableName) {
        if (tableName == null)
            tableName = TMP_TABLE_NAME;
        return tableName;
    }

    /**
     * A helper function used to truncate the semicolon of the query if exists
     *
     * @param query the query to be truncated
     * @return a query with semicolon truncated
     */
    private static String truncateSemicolon(String query) {
        query = query.trim();
        if (query.endsWith(";"))
            return query.substring(0, query.length() - 1);
        return query;
    }

    /**
     * Convert an integer array to a String
     * <p>
     * new Integer[]{1, 2, 3} => 1, 2, 3
     *
     * @param array the array to be converted
     * @return the resulting String
     */
    private static String IntArrayToString(Integer[] array) {
        String string = Arrays.toString(array);
        return string.substring(1, string.length() - 1);
    }
}

/**
 * The class is used to print the result set
 */
class ResultPrinter {
    /**
     * This method prints out the content of a result set
     *
     * @param rs the table to be output (query result)
     * @throws SQLException
     */
    static void print(ResultSet rs) throws SQLException {
        printHeader(rs);
        while (rs.next())
            printRow(rs);
    }

    /**
     * This method prints the header of the table to be output.
     *
     * @param rs the table to be output (query result)
     * @throws SQLException
     */
    private static void printHeader(ResultSet rs) throws SQLException {
        int columnCount = rs.getMetaData().getColumnCount();
        for (int i = 1; i <= columnCount; i++) {
            String columnName = rs.getMetaData().getColumnName(i);
            System.out.print(columnName + "\t");
        }
        System.out.println();
    }

    /**
     * Print the result selected from the result set in the format of each row
     * corresponding one data entry
     *
     * @param rs the table to be output (query result)
     * @throws SQLException
     */
    private static void printRow(ResultSet rs) throws SQLException {
        int columnCount = rs.getMetaData().getColumnCount();
        for (int i = 1; i <= columnCount; i++) {
            String className = rs.getMetaData().getColumnClassName(i);
            if ("java.lang.Integer".equals(className)) {
                System.out.print(rs.getInt(i));
            } else if ("java.lang.Long".equals(className)) {
                System.out.print(rs.getLong(i));
            } else if ("java.lang.Float".equals(className)) {
                System.out.print(rs.getFloat(i));
            } else if ("java.sql.Date".equals(className)) {
                System.out.print(rs.getDate(i));
            } else if ("java.lang.Boolean".equals(className)) {
                System.out.print(rs.getBoolean(i));
            } else if ("java.lang.String".equals(className)) {
                System.out.print(rs.getString(i));
            } else {
                throw new IllegalStateException("Unexpected value: " + className);
            }
            System.out.print("\t");
        }
        System.out.println();
    }
}