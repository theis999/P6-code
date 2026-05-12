using Microsoft.AspNetCore.Mvc;
using Microsoft.AspNetCore.Authentication;
using Microsoft.AspNetCore.Authorization;
using Microsoft.AspNetCore.Mvc.RazorPages;
using System.Net.Http;
using System.Security.Claims;
using System;
using System.Text;
using System.Text.Json;
using System.Text.Json.Nodes;
using System.Diagnostics;


namespace newnewWebinterface.Pages;

[Authorize]
public class RegisterModel : PageModel
{
    public string name;
    public string ProductID;

    public async Task<IActionResult> OnPostAsync()
    {
        string authentikUserID = User.FindFirstValue("sub");
        string authentikUserName = User.FindFirstValue("name");

        using StringContent jsonContent = new(
         JsonSerializer.Serialize(new
         {
             name = name,
             product_id = ProductID,
              user = new {
                name = authentikUserName,
                authentik_user_id = authentikUserID
        }
             
         }),
        System.Text.Encoding.UTF8,
        "application/json");

        var client = new HttpClient();
        using HttpResponseMessage response = await client.PostAsync("https://smakdb.head9x.dk/boards", jsonContent);


        System.Diagnostics.Debug.WriteLine("Response: "+ response);


        return RedirectToPage("./Index");
    }
}
