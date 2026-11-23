fun readNumber(reader: BufferedReader): Int?{
    try{
        val line = reader.readLine()
        return  Integer.parseInt(line)
    }
    catch{
        //return null
    }
    finally{
        reader.close()
    }

	val a: Int? = try { parseInt(input) } catch (e: NumberFormatException) {  }
	return null
}