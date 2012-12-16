package uk.ac.cam.tcs40.sbus.airs.sensor;

import android.database.Cursor;
import android.database.sqlite.SQLiteDatabase;
import android.os.Environment;

public class AirsDb {


	private static String m_AirsDbPath = Environment.getExternalStorageDirectory().getAbsolutePath() + "/Android/data/com.airs/files/AIRS_backup.db"; 
	private SQLiteDatabase m_Database; 

	public void open() {
		m_Database = SQLiteDatabase.openDatabase(m_AirsDbPath, null, SQLiteDatabase.OPEN_READONLY);
	}

	public Cursor query() {
		String sql = "SELECT Timestamp, Symbol, Value " +
						"FROM 'airs_values' " +
						"WHERE Symbol = 'BV' " +
						"ORDER BY Timestamp ASC " +
						"LIMIT 0, 10";
		Cursor values = m_Database.rawQuery(sql, null);	
		return values;
	}

	public void close() 
	{
		if(m_Database != null)
			m_Database.close();
	}
}
